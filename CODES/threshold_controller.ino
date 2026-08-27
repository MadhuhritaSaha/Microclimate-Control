/*
 * AI-Powered Microclimate Control for Smart Indoor Farming
 * Stage 1 firmware: literature-derived chickpea (Cicer arietinum) thresholds
 * Board: ESP32 DevKit V1 (30-pin)
 *
 * Libraries (Arduino Library Manager):
 *   - DHT sensor library          by Adafruit
 *   - Adafruit Unified Sensor
 *   - LiquidCrystal_I2C           by Frank de Brabander
 *   - NTPClient                   by Fabrice Weinberg   (optional, for photoperiod)
 *
 * Open Serial Monitor at 115200 baud.
 * Every actuator change prints an English reason — this is XAI layer 0.
 */

#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <DHT.h>
#include <WiFi.h>
#include <time.h>

// -------------------- USER SETTINGS --------------------
static const char *WIFI_SSID = "YOUR_WIFI_SSID";
static const char *WIFI_PASS = "YOUR_WIFI_PASSWORD";
static const bool  USE_WIFI  = false;   // set true after the box works offline

// I2C LCD address: run the scanner sketch if 0x27 shows nothing. Try 0x3F.
static const uint8_t LCD_ADDR = 0x27;

// Soil calibration — REPLACE after running sensor_calibration.ino
// dryADC = probe in air (or oven-dry pot). wetADC = probe in saturated pot.
static const int SOIL_DRY_ADC = 4095;
static const int SOIL_WET_ADC = 1200;

// LDR calibration — REPLACE after calibration
// darkADC = lid closed at night. lightADC = lid open under the LED / window.
static const int LDR_DARK_ADC  = 3300;
static const int LDR_LIGHT_ADC = 2500;
static const int LDR_DARK_TRIP = 2900;

// Photoperiod in Asia/Kolkata (IST = UTC+5:30)
static const int PHOTO_ON_HOUR  = 6;     // 06:00
static const int PHOTO_OFF_HOUR = 20;    // 20:00   → 14 h

// Chickpea bands (see docs / Figure 4)
static const float T_FAN_ON_C      = 28.0;
static const float T_FAN_OFF_C     = 26.0;
static const float T_HEAT_ALERT_C  = 32.0;
static const float T_COLD_C        = 18.0;

static const float RH_MIST_ON      = 38.0;
static const float RH_MIST_OFF     = 42.0;
static const float RH_FAN_ON       = 65.0;
static const float RH_FAN_OFF      = 60.0;

static const int   SM_PUMP_ON_PCT  = 38;
static const int   SM_PUMP_OFF_PCT = 45;

static const uint32_t PUMP_PULSE_MS      = 8000;     // 8 s
//static const uint32_t PUMP_COOLDOWN_MS   = 600000;   // 10 min
static const uint32_t PUMP_COOLDOWN_MS = 20000;  // 20 s while testing, was 600000
static const uint32_t MIST_PULSE_MS      = 12000;    // 12 s
static const uint32_t MIST_COOLDOWN_MS   = 90000;    // 90 s
static const uint32_t LOOP_MS            = 3000;
// -------------------------------------------------------

// Pins — ADC1 only for analog (Wi-Fi safe)
static const int PIN_DHT        = 4;
static const int PIN_SOIL       = 34;
static const int PIN_LDR        = 35;
static const int PIN_RELAY_MIST = 26;   // IN1
static const int PIN_RELAY_PUMP = 27;   // IN2
static const int PIN_RELAY_FAN  = 25;   // IN3
static const int PIN_RELAY_LED  = 33;   // IN4

// Most cheap 5 V 4-ch relay boards are ACTIVE LOW.
static const bool RELAY_ACTIVE_LOW = true;

#define DHTTYPE DHT11
DHT dht(PIN_DHT, DHTTYPE);
LiquidCrystal_I2C lcd(LCD_ADDR, 16, 2);

struct Actuator {
  const char *name;
  int pin;
  bool on;
};

Actuator mist = {"HUMIDIFIER", PIN_RELAY_MIST, false};
Actuator pump = {"WATER_PUMP", PIN_RELAY_PUMP, false};
Actuator fan  = {"VENT_FAN",   PIN_RELAY_FAN,  false};
Actuator led  = {"LED_STRIP",  PIN_RELAY_LED,  false};

uint32_t lastPumpOffMs = 0;
uint32_t lastMistOffMs = 0;
uint32_t pumpOnSince   = 0;
uint32_t mistOnSince   = 0;

void relayWrite(Actuator &a, bool turnOn, const char *reason) {
  if (a.on == turnOn) return;
  a.on = turnOn;
  int level = turnOn ? (RELAY_ACTIVE_LOW ? LOW : HIGH)
                     : (RELAY_ACTIVE_LOW ? HIGH : LOW);
  digitalWrite(a.pin, level);
  Serial.print("[XAI] ");
  Serial.print(a.name);
  Serial.print(turnOn ? " ON  — " : " OFF — ");
  Serial.println(reason);
}

int soilPercent(int adc) {
  long pct = map(adc, SOIL_DRY_ADC, SOIL_WET_ADC, 0, 100);
  if (pct < 0) pct = 0;
  if (pct > 100) pct = 100;
  return (int)pct;
}

bool inPhotoperiod(struct tm *ti) {
  // If NTP failed, default to "daytime" so the plant is not left in the dark
  // during a weekend demo. Change to false if you prefer fail-safe lights-off.
  if (ti == nullptr) return true;
  int minutes = ti->tm_hour * 60 + ti->tm_min;
  int on  = PHOTO_ON_HOUR  * 60;
  int off = PHOTO_OFF_HOUR * 60;
  return minutes >= on && minutes < off;
}

void setupWifiAndTime() {
  if (!USE_WIFI) {
    Serial.println("Wi-Fi disabled. Photoperiod uses fail-open (treat as day).");
    return;
  }
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.print("Wi-Fi");
  uint32_t t0 = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - t0 < 15000) {
    delay(400);
    Serial.print(".");
  }
  Serial.println();
  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("IP ");
    Serial.println(WiFi.localIP());
    // IST = UTC+5:30 = 19800 s
    configTime(19800, 0, "pool.ntp.org", "time.nist.gov");
  } else {
    Serial.println("Wi-Fi failed — continuing offline.");
  }
}

void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.println("\n=== Chickpea microclimate controller ===");

  analogSetAttenuation(ADC_11db);   // ~0–3.3 V on ADC

  pinMode(PIN_RELAY_MIST, OUTPUT);
  pinMode(PIN_RELAY_PUMP, OUTPUT);
  pinMode(PIN_RELAY_FAN,  OUTPUT);
  pinMode(PIN_RELAY_LED,  OUTPUT);

  // Force all relays OFF at boot (active-low → write HIGH)
  digitalWrite(PIN_RELAY_MIST, RELAY_ACTIVE_LOW ? HIGH : LOW);
  digitalWrite(PIN_RELAY_PUMP, RELAY_ACTIVE_LOW ? HIGH : LOW);
  digitalWrite(PIN_RELAY_FAN,  RELAY_ACTIVE_LOW ? HIGH : LOW);
  digitalWrite(PIN_RELAY_LED,  RELAY_ACTIVE_LOW ? HIGH : LOW);

  dht.begin();
  Wire.begin(21, 22);
  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Chickpea box");
  lcd.setCursor(0, 1);
  lcd.print("booting...");

  setupWifiAndTime();
  lastPumpOffMs = millis();
  lastMistOffMs = millis();
}

void loop() {
  float t  = dht.readTemperature();
  float rh = dht.readHumidity();
  int soilAdc = analogRead(PIN_SOIL);
  int ldrAdc  = analogRead(PIN_LDR);
  int sm = soilPercent(soilAdc);
  bool dark = ldrAdc >= LDR_DARK_TRIP;

  struct tm ti;
  bool haveTime = getLocalTime(&ti, 50);
  bool day = inPhotoperiod(haveTime ? &ti : nullptr);

  if (isnan(t) || isnan(rh)) {
    Serial.println("DHT11 read failed — check wiring / 2 s spacing.");
    lcd.setCursor(0, 0);
    lcd.print("DHT11 fail      ");
    delay(LOOP_MS);
    return;
  }

  // ---------- FAN : high T OR high RH ----------
  bool wantFan = false;
  const char *fanWhy = "bands OK";
  if (t >= T_FAN_ON_C) {
    wantFan = true;
    fanWhy = "T>=28C chickpea heat band";
  } else if (rh >= RH_FAN_ON) {
    wantFan = true;
    fanWhy = "RH>=65% disease-risk band";
  }
  if (fan.on && t <= T_FAN_OFF_C && rh <= RH_FAN_OFF) {
    wantFan = false;
    fanWhy = "T<=26C and RH<=60% hysteresis";
  } else if (fan.on && !wantFan) {
    // still inside hysteresis — keep previous
    wantFan = true;
    fanWhy = "hysteresis hold";
  }
  if (t >= T_HEAT_ALERT_C) {
    wantFan = true;
    fanWhy = "T>=32C HEAT STRESS alert";
  }
  relayWrite(fan, wantFan, fanWhy);

  // ---------- HUMIDIFIER : only when air is actually dry ----------
  uint32_t now = millis();
  if (mist.on && (now - mistOnSince >= MIST_PULSE_MS)) {
    relayWrite(mist, false, "12 s pulse finished");
    lastMistOffMs = now;
  } else if (!mist.on) {
    if (rh <= RH_MIST_ON && (now - lastMistOffMs >= MIST_COOLDOWN_MS)) {
      relayWrite(mist, true, "RH<=38% dry air, 12 s mist pulse");
      mistOnSince = now;
    }
  }
  if (mist.on && rh >= RH_MIST_OFF) {
    relayWrite(mist, false, "RH>=42% hysteresis");
    lastMistOffMs = now;
  }

  // ---------- PUMP : pulse + lockout so a bottle tank cannot flood ----------
  if (pump.on && (now - pumpOnSince >= PUMP_PULSE_MS)) {
    relayWrite(pump, false, "8 s pulse finished, 10 min lockout");
    lastPumpOffMs = now;
  } else if (!pump.on) {
    if (sm <= SM_PUMP_ON_PCT && (now - lastPumpOffMs >= PUMP_COOLDOWN_MS)) {
      relayWrite(pump, true, "soil<=38%, 8 s irrigation pulse");
      pumpOnSince = now;
    }
  }
  if (pump.on && sm >= SM_PUMP_OFF_PCT) {
    relayWrite(pump, false, "soil>=45% wet enough");
    lastPumpOffMs = now;
  }

  // ---------- LED : photoperiod AND darkness ----------
  if (day && dark) {
    relayWrite(led, true, "photoperiod 06-20 IST and LDR dark");
  } else if (!day) {
    relayWrite(led, false, "outside 14 h photoperiod, night rest");
  } else {
    relayWrite(led, false, "already bright, LED not needed");
  }

  // Cold note — we have no heater; LED waste heat is a weak bonus already on in day
  if (t < T_COLD_C) {
    Serial.println("[XAI] NOTE — T<18C, no heater in BOM. Keep box away from AC vents.");
  }

  // ---------- LCD ----------
  char l0[17], l1[17];
  snprintf(l0, sizeof(l0), "%4.1fC %4.0f%% %2d%%", t, rh, sm);
  snprintf(l1, sizeof(l1), "F%d P%d H%d L%d %s",
           fan.on ? 1 : 0, pump.on ? 1 : 0, mist.on ? 1 : 0, led.on ? 1 : 0,
           dark ? "dk" : "lt");
  lcd.setCursor(0, 0);
  lcd.print(l0);
  lcd.setCursor(0, 1);
  lcd.print(l1);

  // ---------- CSV log (paste into a spreadsheet) ----------
  Serial.print("CSV,");
  Serial.print(now);
  Serial.print(",");
  Serial.print(t, 1);
  Serial.print(",");
  Serial.print(rh, 1);
  Serial.print(",");
  Serial.print(sm);
  Serial.print(",");
  Serial.print(ldrAdc);
  Serial.print(",");
  Serial.print(fan.on);
  Serial.print(",");
  Serial.print(pump.on);
  Serial.print(",");
  Serial.print(mist.on);
  Serial.print(",");
  Serial.print(led.on);
  Serial.print(",");
  Serial.println(day ? "day" : "night");

  delay(LOOP_MS);
}
