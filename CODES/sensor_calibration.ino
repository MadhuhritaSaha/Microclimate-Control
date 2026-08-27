/*
 * Calibration sketch — run this BEFORE the main controller.
 * Board: ESP32 DevKit V1
 *
 * 1. Soil dry:  probe in air.          Write down AO.
 * 2. Soil wet:  probe in a soaked pot. Write down AO.
 * 3. LDR dark:  cover the LDR.         Write down AO.
 * 4. LDR light: under the LED / window. Write down AO.
 *
 * Paste the four numbers into threshold_controller.ino
 *   SOIL_DRY_ADC, SOIL_WET_ADC, LDR_DARK_ADC, LDR_LIGHT_ADC
 *
 * Typical resistive soil module on 3.3 V / 12-bit ADC:
 *   dry ~ 2800–3600     wet ~ 1100–1800
 * Typical LDR module: higher number = darker.
 */

#include <DHT.h>

static const int PIN_DHT  = 4;
static const int PIN_SOIL = 34;
static const int PIN_LDR  = 35;

DHT dht(PIN_DHT, DHT11);

int avgRead(int pin, int n = 12) {
  long s = 0;
  for (int i = 0; i < n; i++) {
    s += analogRead(pin);
    delay(15);
  }
  return (int)(s / n);
}

void setup() {
  Serial.begin(115200);
  analogSetAttenuation(ADC_11db);
  dht.begin();
  Serial.println("=== calibration ===");
  Serial.println("soil_adc,ldr_adc,T_C,RH_pct");
}

void loop() {
  int soil = avgRead(PIN_SOIL);
  int ldr  = avgRead(PIN_LDR);
  float t  = dht.readTemperature();
  float rh = dht.readHumidity();

  Serial.print(soil);
  Serial.print(",");
  Serial.print(ldr);
  Serial.print(",");
  if (isnan(t)) Serial.print("NaN"); else Serial.print(t, 1);
  Serial.print(",");
  if (isnan(rh)) Serial.println("NaN"); else Serial.println(rh, 1);

  delay(1000);
}
