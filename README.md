# 🌱 Microclimate Control for Smart Indoor Farming

> **AI-Powered Automated Environment Management for Indoor Grow Boxes**  
> Building intelligent microclimate controllers for precision plant cultivation using ESP32, sensor fusion, and threshold-based logic.

![Project Status](https://img.shields.io/badge/status-active-brightgreen)
![Language](https://img.shields.io/badge/language-C++-blue)
![Platform](https://img.shields.io/badge/platform-ESP32-orange)
![License](https://img.shields.io/badge/license-MIT-green)

---

## 📖 Table of Contents

- [Quick Start](#-quick-start)
- [Project Overview](#-project-overview)
- [System Architecture](#-system-architecture)
- [Hardware Setup](#-hardware-setup)
- [Firmware Guide](#-firmware-guide)
- [Sensor Calibration](#-sensor-calibration)
- [Control Logic](#-control-logic)
- [File Structure](#-file-structure)
- [Troubleshooting](#-troubleshooting)
- [Resources](#-resources)

---

## ⚡ Quick Start

**What is this project?**
A fully automated grow box controller that reads environmental sensors and controls four actuators:
- 🌡️ Temperature & humidity (DHT11 sensor)
- 💧 Soil moisture (Resistive soil module)
- 💡 Light levels (LDR sensor)
- 🎛️ Relay-switched: LED strip, ventilation fan, water pump, humidifier

**Minimum setup time:** ~30 minutes for wiring + firmware upload

### 1️⃣ **Flash the Calibration Sketch**
Upload `CODES/sensor_calibration.ino` to your ESP32 first:
```
1. Soil dry:  probe in air          → note ADC value
2. Soil wet:  probe in saturated pot → note ADC value
3. LDR dark:  cover the sensor       → note ADC value
4. LDR light: place under LED        → note ADC value
```

### 2️⃣ **Update Thresholds**
Edit `CODES/threshold_controller.ino` lines 32–38:
```cpp
static const int SOIL_DRY_ADC = YOUR_DRY_VALUE;    // e.g., 3600
static const int SOIL_WET_ADC = YOUR_WET_VALUE;    // e.g., 1200
static const int LDR_DARK_ADC  = YOUR_DARK_VALUE;  // e.g., 3300
static const int LDR_LIGHT_ADC = YOUR_LIGHT_VALUE; // e.g., 2500
```

### 3️⃣ **Wire Your Hardware**
See [Hardware Setup](#-hardware-setup) section. **Golden rule:** 
- Sensors on **3.3 V** (ADC safe)
- Actuators on **5 V or 12 V** (via relay, not ESP32 pins)

### 4️⃣ **Upload Main Firmware**
Upload `threshold_controller.ino` and open Serial Monitor at **115200 baud**.

✅ **You're live!** LCD displays live sensor readings. Serial prints every actuator decision with a reasoning message (XAI layer 0).

---

## 🎯 Project Overview

### **What Problem Does This Solve?**
Indoor plant farming requires precise environmental control:
- **Too hot?** → Mold, reduced yield
- **Too dry?** → Wilting
- **Too wet?** → Root rot
- **Wrong photoperiod?** → Flowering fails

Traditional grow boxes use dumb timers or manual intervention. This controller automates the entire process using **science-backed thresholds** for chickpea (*Cicer arietinum*).

### **Key Features**
✨ **Intelligent Control:**
- Hysteresis-based switching (prevents relay chatter)
- Photoperiod automation (6 AM – 8 PM IST for chickpea)
- Pulsed irrigation (8 s on, 10 min lockout to prevent flooding)
- Temperature + humidity + soil moisture + light fusion

🔍 **Explainable AI (XAI):**
- Every relay action prints a reason to Serial
- CSV logging for data analysis and debugging
- 16×2 LCD live readout

🌍 **Fault Tolerance:**
- Wi-Fi optional (offline-first design)
- Graceful sensor failure handling
- Fail-safe photoperiod (treats missed NTP as "daytime")

---

## 🏗️ System Architecture

### **Block Diagram**
```
┌─────────────────────────────────────┐
│         SENSORS (3.3 V)             │
├─────────────────────────────────────┤
│ • DHT11       → GPIO 4   (Temp/RH)  │
│ • Soil module → GPIO 34  (Moisture) │
│ • LDR module  → GPIO 35  (Light)    │
└────────────────┬────────────────────┘
                 │
          ┌──────▼──────┐
          │   ESP32     │
          │  DevKit V1  │◄─── Laptop USB (power + debug)
          │  (Logic)    │
          └──────┬──────┘
                 │
        ┌────────▼────────┐
        │   4-ch Relay    │
        │   HW-316        │
        │  (Switches)     │
        └────────┬────────┘
                 │
   ┌─────────────┼─────────────┐
   │             │             │
   ▼             ▼             ▼
 CH1           CH2            CH3
 IN1 (26)      IN2 (27)       IN3 (25)      IN4 (33)
   │             │             │             │
   ▼             ▼             ▼             ▼
HUMIDIFIER    PUMP          FAN            LED
 5V USB       5V 2A        12V 8021        12V/5V
              adapter      fan             strip

     I2C (GPIO 21/22)
            │
            ▼
        LCD 16×2
        (Display)
```

### **Data Flow**
```
LOOP (every 3 seconds):
  1. Read sensors → T, RH, Soil %, Light
  2. Query NTP → Get hour (photoperiod)
  3. Apply thresholds → Decide each actuator
  4. Send relay commands (if changed)
  5. Update LCD + Serial CSV
  6. Sleep 3 s → repeat
```

### **Architecture Documents**
- 📄 **`SYSTEM ARCHITECTURE/system_architecture.png`** — Block diagram & signal flow
- 📄 **`SYSTEM ARCHITECTURE/growbox_concept.png`** — Physical layout inside the box

---

## 🔌 Hardware Setup

### **Bill of Materials (BOM)**

| Component | Qty | Typical Cost | Notes |
|-----------|-----|--------------|-------|
| **ESP32 DevKit V1 (30-pin)** | 1 | $8–12 | Doitinkl or AZ-Delivery |
| **DHT11 module (pre-soldered)** | 1 | $2–3 | Temp + humidity combo |
| **Resistive soil moisture module** | 1 | $2–3 | Analog output, 3.3 V safe |
| **LM393 LDR module (analog + digital)** | 1 | $2–3 | Use AO pin only |
| **16×2 LCD + I2C backpack** | 1 | $5–7 | Pre-soldered back (no breadboard wiring) |
| **4-ch 5 V relay HW-316** | 1 | $5–8 | Active-LOW, JD-VCC jumper ON |
| **12 V 2 A power adapter** | 1 | $8–12 | For fan + LED (if 12 V) |
| **5 V 2 A USB charger** | 2 | $3–5 ea | One for pump, one for humidifier |
| **12 V 8021 axial fan** | 1 | $5–8 | OR 5 V depending on PSU |
| **USB ultrasonic humidifier** | 1 | $8–15 | OR 5 V mini sprayer |
| **5 V mini peristaltic pump** | 1 | $8–12 | Dosing pump for irrigation |
| **12 V RGB LED strip (or 5 V)** | 1 | $5–10 | Grow light simulation |
| **Breadboard + jumpers** | — | $3–5 | For sensor wiring only |
| **Resistors, capacitors (optional)** | — | $1–2 | For debouncing (not required) |
| **USB-C or Micro-USB cable** | 1 | $1–2 | Programming cable |

**Total:** ~$70–120 for a complete working system.

### **Step-by-Step Wiring**

#### **Power Rails (Critical)**
1. **Star GND:** Join these in ONE node:
   - ESP32 `GND`
   - Relay `GND`
   - 12 V adapter `−`
   - 5 V charger `−` (all three chargers if you have them)

2. **ESP32 Power:**
   - USB or any 5 V source → Micro-USB (logic only, NOT pump)
   - ESP32 `3V3` → Sensor power rails (DHT11, soil, LDR)
   - ESP32 `5V` → Relay coil `VCC`

3. **External Power (Separate):**
   - 12 V adapter → Relay COM pins (CH3 fan, CH4 LED if 12 V)
   - 5 V chargers → Relay COM pins (CH1 humidifier, CH2 pump)
   - **DO NOT** bridge these together—use separate adapters.

#### **Sensors (All 3.3 V)**

| Sensor | ESP32 Pin | Remarks |
|--------|-----------|---------|
| **DHT11** |  |  |
| — VCC | `3V3` | Red wire |
| — DATA | `GPIO 4` | Yellow wire |
| — GND | `GND` | Black wire |
| **Soil module** |  |  |
| — VCC | `3V3` | Red, NOT 5V (ADC safe) |
| — AO | `GPIO 34` (ADC1) | Green |
| — GND | `GND` | Black |
| — DO | (not connected) | Ignore |
| **LDR module** |  |  |
| — VCC | `3V3` | Red |
| — AO | `GPIO 35` (ADC1) | Orange |
| — GND | `GND` | Black |
| — DO | (not connected) | Ignore |

#### **Relay Logic Header (Active-LOW, JD-VCC ON)**

| Relay Pin | ESP32 GPIO | Purpose | Load Voltage |
|-----------|-----------|---------|--------------|
| GND | GND | Common | — |
| IN1 | `GPIO 26` | Humidifier | 5 V |
| IN2 | `GPIO 27` | Water pump | 5 V |
| IN3 | `GPIO 25` | Vent fan | 12 V |
| IN4 | `GPIO 33` | LED strip | 5 V or 12 V |
| VCC | ESP32 `5V` | Coil supply | — |

#### **LCD + I2C Backpack**

| Backpack Pin | ESP32 GPIO | Wire Color |
|--------------|-----------|-----------|
| VCC | `5V` | Red |
| GND | `GND` | Black |
| SDA | `GPIO 21` | Blue |
| SCL | `GPIO 22` | Purple |

Default I2C address: `0x27` (try `0x3F` if backlight doesn't work).

#### **Actuator Load Sides**

All channels use **COM (common)** and **NO (normally open)** terminals only—**never NC**.

**Example: 12 V fan (CH3)**
```
12V adapter (+) ──→ Relay CH3 COM
                    Relay CH3 NO ──→ Fan red (+)
Fan black (−) ───────────────────→ 12V adapter (−) → Star GND
```

**See full details in:** `SCHEMATIC/WIRING_CONNECTION_TABLE.md`

### **Visual Schematics**
- 📄 **`SCHEMATIC/Schematic.png`** — Full wiring diagram
- 📄 **`SCHEMATIC/Schematic V2.jpeg`** — Revised version

---

## 💻 Firmware Guide

### **Prerequisites**
Install in Arduino IDE's Library Manager:
1. **DHT sensor library** by Adafruit
2. **Adafruit Unified Sensor** by Adafruit
3. **LiquidCrystal_I2C** by Frank de Brabander
4. **NTPClient** by Fabrice Weinberg (optional, for network time)

### **File Structure**

```
CODES/
├── sensor_calibration.ino      ← Run this FIRST
└── threshold_controller.ino    ← Run this SECOND (main loop)
```

### **Step 1: Calibration Sketch** (`sensor_calibration.ino`)

**Purpose:** Determine ADC values for your specific sensors (every sensor is slightly different).

**Process:**
1. Upload `sensor_calibration.ino` to ESP32
2. Open Serial Monitor (115200 baud)
3. You'll see CSV output: `soil_adc, ldr_adc, T_C, RH_pct`
4. Perform four tests:
   - **Soil dry:** Hold probe in air (or oven-dry pot)
   - **Soil wet:** Submerge probe in water-saturated pot
   - **LDR dark:** Cover sensor with your hand / opaque object
   - **LDR light:** Place sensor under grow LED / window

5. **Record four values:**
   ```
   SOIL_DRY_ADC  = [value from step 1]
   SOIL_WET_ADC  = [value from step 2]
   LDR_DARK_ADC  = [value from step 3]
   LDR_LIGHT_ADC = [value from step 4]
   ```

**Typical ranges (ESP32 12-bit ADC, 3.3 V):**
- Soil dry: 2800–3600
- Soil wet: 1100–1800
- LDR dark: 2500–3500 (higher = darker)
- LDR light: 500–2000

### **Step 2: Main Controller** (`threshold_controller.ino`)

**Sections to customize (lines 22–65):**

```cpp
// USER SETTINGS
static const char *WIFI_SSID = "YOUR_SSID";         // your Wi-Fi network
static const char *WIFI_PASS = "YOUR_PASSWORD";     // your Wi-Fi password
static const bool  USE_WIFI  = false;               // disable until box works offline

// Calibration values (from step 1)
static const int SOIL_DRY_ADC = 4095;               // REPLACE!
static const int SOIL_WET_ADC = 1200;               // REPLACE!
static const int LDR_DARK_ADC  = 3300;              // REPLACE!
static const int LDR_LIGHT_ADC = 2500;              // REPLACE!

// Photoperiod (for Asia/Kolkata IST = UTC+5:30)
static const int PHOTO_ON_HOUR  = 6;                // 06:00 (6 AM)
static const int PHOTO_OFF_HOUR = 20;               // 20:00 (8 PM) → 14 h day

// Chickpea thresholds (from literature)
static const float T_FAN_ON_C      = 28.0;          // Turn on fan at 28°C
static const float T_FAN_OFF_C     = 26.0;          // Turn off fan below 26°C (hysteresis)
static const float T_HEAT_ALERT_C  = 32.0;          // Stress alarm at 32°C
static const float T_COLD_C        = 18.0;          // Note: no heater, keep away from AC

static const float RH_MIST_ON      = 38.0;          // Humidify below 38% RH
static const float RH_MIST_OFF     = 42.0;          // Stop humidifying above 42% RH
static const float RH_FAN_ON       = 65.0;          // Exhaust above 65% (mold risk)
static const float RH_FAN_OFF      = 60.0;          // Hysteresis: stop below 60%

static const int   SM_PUMP_ON_PCT  = 38;            // Irrigate when <38% soil moisture
static const int   SM_PUMP_OFF_PCT = 45;            // Stop watering at ≥45%

static const uint32_t PUMP_PULSE_MS    = 8000;      // Water for 8 seconds
static const uint32_t PUMP_COOLDOWN_MS = 600000;    // Wait 10 min before next pulse (prevents flooding)
static const uint32_t MIST_PULSE_MS    = 12000;     // Mist for 12 seconds
static const uint32_t MIST_COOLDOWN_MS = 90000;     // Wait 90 s before next mist
static const uint32_t LOOP_MS           = 3000;     // Read sensors every 3 seconds
```

**Upload & Test:**
```
1. Compile & upload
2. Open Serial Monitor (115200 baud)
3. Watch for: [XAI] messages explaining each relay toggle
4. Check LCD for live readings
5. Paste CSV data into a spreadsheet to verify sensor range
```

### **Serial Output Examples**

Every decision prints with a reason (XAI layer 0):

```
=== Chickpea microclimate controller ===
Wi-Fi disabled. Photoperiod uses fail-open (treat as day).

[XAI] VENT_FAN ON  — T>=28C chickpea heat band
[XAI] WATER_PUMP ON  — soil<=38%, 8 s irrigation pulse
[XAI] WATER_PUMP OFF — 8 s pulse finished, 10 min lockout
[XAI] HUMIDIFIER ON  — RH<=38% dry air, 12 s mist pulse
[XAI] HUMIDIFIER OFF — 12 s pulse finished
[XAI] LED_STRIP ON  — photoperiod 06-20 IST and LDR dark

CSV,12345,24.5,42.0,45,2850,1,1,1,1,day
```

The **CSV line** can be pasted directly into Excel:
- Timestamp (ms)
- Temperature (°C)
- Humidity (%)
- Soil moisture (%)
- LDR ADC value
- Fan state (1=on)
- Pump state (1=on)
- Mist state (1=on)
- LED state (1=on)
- Photoperiod (day/night)

---

## 📏 Sensor Calibration

### **Why Calibrate?**
Analog sensors have factory tolerance (±10–20%). Calibration maps raw ADC to physical units:
- **Soil:** ADC 4095 (dry) → 0% moisture; ADC 1200 (wet) → 100% moisture
- **LDR:** ADC 3300 (dark) is a trip point for "night"

### **Detailed Calibration Process**

#### **Soil Moisture**
1. Get a small pot with dry soil (or oven-dry at 105°C for lab accuracy)
2. Insert the soil probe
3. Note ADC value (expected ~2800–3600)
4. **Now water the pot thoroughly** until water drains from the bottom
5. Insert probe into the saturated soil
6. Note ADC value (expected ~1100–1800)

→ Update `SOIL_DRY_ADC` and `SOIL_WET_ADC` in the firmware.

The code maps linearly:
```cpp
int soilPercent(int adc) {
  long pct = map(adc, SOIL_DRY_ADC, SOIL_WET_ADC, 0, 100);
  // Clamp to [0, 100]
  if (pct < 0) pct = 0;
  if (pct > 100) pct = 100;
  return (int)pct;
}
```

#### **Light Level (LDR)**
1. Cover the LDR with your hand or a cup (complete darkness)
2. Note ADC value (expected ~2500–3500; higher = darker)
3. Move to a bright room or hold under the LED
4. Note ADC value (expected ~500–2000)

→ Update `LDR_DARK_ADC` and `LDR_LIGHT_ADC`.

The firmware uses a trip point: if ADC ≥ `LDR_DARK_TRIP`, it's considered "dark". Example:
```cpp
static const int LDR_DARK_TRIP = 2900;  // e.g., above 2900 ADC = "night"
bool dark = ldrAdc >= LDR_DARK_TRIP;
```

**Tip:** Run calibration sketch once per setup or if sensors are swapped.

---

## 🎛️ Control Logic

### **Decision Tree**

The firmware implements **hysteresis-based state machines** to prevent relay chatter:

#### **1. Ventilation Fan (Temperature + Humidity Exhaust)**
```
IF T ≥ 28°C
  → Turn ON (chickpea heat band)
ELSE IF RH ≥ 65%
  → Turn ON (mold prevention)
ELSE IF (fan is ON) AND (T ≤ 26°C AND RH ≤ 60%)
  → Turn OFF (hysteresis: wait for both thresholds)
ELSE IF (fan is ON)
  → KEEP ON (hold inside hysteresis band)

IF T ≥ 32°C
  → Force ON + print "HEAT STRESS alert"
```

**Why hysteresis?** Without it, a fan would oscillate 28.0°C ↔ 27.9°C ↔ 28.0°C every second, wearing out the relay.

#### **2. Water Pump (Soil Moisture + Pulse + Lockout)**
```
IF soil ≤ 38%
  AND 10 min cooldown has elapsed
  → Turn ON for 8 seconds (PUMP_PULSE_MS)
     Then automatically OFF → start cooldown again

IF pump is ON
  AND 8 s pulse finished
  → Turn OFF + lock for 10 min (PUMP_COOLDOWN_MS)

IF pump is ON
  AND soil ≥ 45%
  → Turn OFF early (wet enough)
     Lock for 10 min
```

**Why pulse + lockout?** A 5 V peristaltic pump on a small water bottle tank can flood the plant if left on. Pulsing prevents overflow.

#### **3. Humidifier (RH Control + Pulse)**
```
IF RH ≤ 38%
  AND 90 s cooldown has elapsed
  → Turn ON for 12 seconds (MIST_PULSE_MS)

IF humidifier is ON
  AND 12 s pulse finished
  → Turn OFF + lock for 90 s (MIST_COOLDOWN_MS)

IF humidifier is ON
  AND RH ≥ 42% (hysteresis)
  → Turn OFF
     Lock for 90 s
```

#### **4. LED Strip (Photoperiod + Darkness)**
```
IF (Hour between 06:00 and 20:00)
  AND LDR detects darkness
  → Turn ON (supplemental grow light)
ELSE
  → Turn OFF

IF (Hour outside photoperiod)
  → Turn OFF (plant needs dark period for rest)
ELSE IF (LDR detects light)
  → Turn OFF (natural/ambient light sufficient)
```

**Photoperiod default:** 6 AM – 8 PM IST (14 hours). Edit `PHOTO_ON_HOUR` and `PHOTO_OFF_HOUR` for your timezone.

### **Cold Snap Alert (No Heater)**
```
IF T < 18°C
  → Print: "T<18C, no heater in BOM. Keep box away from AC vents."
  (no action; relies on operator)
```

---

## 📁 File Structure

```
Microclimate-Control/
│
├── README.md                                    ← You are here
│
├── CODES/
│   ├── sensor_calibration.ino                  ← Step 1: Calibrate sensors
│   └── threshold_controller.ino                ← Step 2: Main controller
│
├── SCHEMATIC/
│   ├── Schematic.png                           ← Full wiring diagram (high-res)
│   ├── Schematic V2.jpeg                       ← Revised schematic
│   └── WIRING_CONNECTION_TABLE.md              ← Point-to-point wiring table
│
├── SYSTEM ARCHITECTURE/
│   ├── system_architecture.png                 ← Block diagram & signal flow
│   └── growbox_concept.png                     ← Physical layout mockup
│
├── Project report on Microclimate Control.pdf  ← Full technical report
└── Ai Tech - A1 Poster_*.pdf                  ← Conference/exhibition poster
```

### **Key Files**

| File | Purpose | Read If... |
|------|---------|-----------|
| `threshold_controller.ino` | Main logic | You want to understand or customize the control thresholds |
| `sensor_calibration.ino` | Calibration tool | Your sensors are not responding correctly |
| `WIRING_CONNECTION_TABLE.md` | Wiring reference | You need to connect hardware or debug no/mixed signals |
| `system_architecture.png` | System overview | You want to see the big picture |
| `Project report...pdf` | Full documentation | You need background, literature, or tuning methodology |

---

## 🔧 Troubleshooting

### **LCD Shows Nothing**
**Diagnosis:**
1. Are the backpack `SDA` and `SCL` wires connected (GPIO 21, 22)?
2. Is the backpack powered (red wire to 5V)?
3. Is the address correct (0x27 or 0x3F)?

**Fix:**
1. Double-check I2C address. Upload the **I2C Scanner sketch** (find in Arduino examples).
2. If scanner finds no devices, check power and GND star node.
3. Try `LCD_ADDR = 0x3F` if 0x27 doesn't work.

---

### **DHT11 Reads as NaN (Temperature and Humidity)**
**Likely causes:**
1. Wiring: DATA line on wrong GPIO (should be GPIO 4)
2. Power: VCC on 5V instead of 3.3V (DHT11 is 3.3 V only)
3. Timing: Library needs ≥2 seconds between reads (firmware does 3 s loop—OK)

**Fix:**
1. Check wiring against the table.
2. If already correct, try a fresh DHT11 (they occasionally fail in transit).

---

### **Soil Sensor Reads Constantly 0% or 100%**
**Likely causes:**
1. Calibration values are wrong (SOIL_DRY_ADC or SOIL_WET_ADC)
2. Sensor never reaches those extremes in your setup

**Fix:**
1. Re-run `sensor_calibration.ino` and verify the dry/wet ADC ranges.
2. Update firmware with correct values.
3. Check the map function logic in `soilPercent()`.

---

### **Relay Clicks But Actuator Doesn't Turn On**
**Likely causes:**
1. Relay terminal wired to NC (normally closed) instead of NO
2. Load power not connected or polarity reversed
3. Relay COM terminal floating (not connected to +voltage)

**Fix:**
1. Check you're using COM + NO, **never NC**.
2. Verify load power polarity (red to +, black to −).
3. Ensure 12V or 5V adapter is actually powered on.
4. Try a different load (e.g., swap fan and LED to test relays).

---

### **Pump Floods the Plant; Irrigation Won't Stop**
**Likely causes:**
1. Water bottle tank too large or no outlet control
2. PUMP_COOLDOWN_MS is too short (default 10 min is safe)
3. Pump relay stuck on (wiring or relay failure)

**Fix:**
1. Use a smaller bottle or add a manual overflow valve.
2. Increase cooldown: `PUMP_COOLDOWN_MS = 900000;` (15 min).
3. Move water bottle away from soil to reduce flooding risk.
4. Test relay manually: does it release when you cut GPIO 27 power?

---

### **Wi-Fi Keeps Reconnecting**
**Likely cause:** NTPClient timeout is causing sync loop.

**Fix:**
```cpp
// Temporarily disable Wi-Fi (line 25):
static const bool USE_WIFI = false;  // ← change from true to false
```
The firmware will operate offline with fail-safe photoperiod logic.

---

### **Serial Monitor Freezes or Garbled Text**
**Likely cause:** Baud rate mismatch (should be **115200**).

**Fix:**
1. Open Serial Monitor (Ctrl+Shift+M)
2. Set dropdown to **115200 baud**
3. If still garbled, power-cycle the ESP32

---

### **LED Strip Intermittent / Dim**
**Likely causes:**
1. Relay contact not fully seated (reseat the relay)
2. Power supply current limit (LED strips can draw >1A)
3. Relay worn contacts

**Fix:**
1. Push the relay firmly into the breadboard.
2. Use a dedicated 12 V / 5 V 2 A charger for the strip (not shared).
3. If still dim, test the strip directly on a power supply (bypass relay).

---

## 📚 Resources

### **Hardware Datasheets**
- **ESP32 DevKit V1:** [Espressif ESP32 Datasheet](https://www.espressif.com/sites/default/files/documentation/esp32_datasheet_en.pdf)
- **DHT11:** [DHT11 Datasheet](https://www.mouser.com/datasheet/2/758/DHT11-1117069-0.pdf)
- **HW-316 Relay Module:** [Pinout Diagram](https://lastminuteengineers.com/arduino-relay-control-tutorial/) (or search "HW-316 relay")
- **LDR LM393 Module:** Generic light sensor; AO pin = analog output

### **Arduino Libraries**
- **DHT sensor library:** https://github.com/adafruit/DHT-sensor-library
- **LiquidCrystal_I2C:** https://github.com/fdebrabander/Arduino-LiquidCrystal-I2C
- **NTPClient:** https://github.com/taligentx/NTPClient

### **Relevant Literature**
- **Chickpea climate requirements:** Download the full project report (PDF in this repo) for references
- **Grow box basics:** "Plant Factory" by Masahiko Hattori (academic reference)

### **Tools & Utilities**
- **I2C Address Scanner:** Built into Arduino IDE → File → Examples → Wire → i2c_scanner
- **Serial Plotter:** Arduino IDE built-in (Tools → Serial Plotter) for real-time graphing

### **External Links**
- 🌐 [Microclimate Control Project Repository](https://github.com/MadhuhritaSaha/Microclimate-Control)
- 📖 Full technical report: See `Project report on Microclimate Control.pdf`
- 🎨 Exhibition poster: See `Ai Tech - A1 Poster_*.pdf`

---

## 🎓 How to Extend This Project

### **Add AI/ML Predictive Control**
- Stage 1 (current): Threshold-based (rule-based)
- Stage 2 (future): Time-series prediction (LSTM) for anticipatory fan control
- **Reference:** Repo includes research baseline in project report

### **Add IoT Dashboard**
- Stream CSV data to ThingSpeak or Blynk
- Real-time graphing of microclimate over days/weeks
- Remote alerts (Telegram bot, email)

### **Multi-Zone Control**
- Add another ESP32 + relay set for multiple grow boxes
- Central MQTT broker to coordinate lighting & water schedules

### **Adaptive Thresholds**
- ML model learns optimal thresholds per plant species
- Auto-switch between crop profiles (chickpea → tomato → lettuce)

---

## 📝 License & Citation

**This project is open source.** Please cite if used in academic work:
```bibtex
@software{microclimate_control_2025,
  author = {Madhuhrita Saha},
  title = {Microclimate Control for Smart Indoor Farming},
  year = {2025},
  url = {https://github.com/MadhuhritaSaha/Microclimate-Control}
}
```

---

## 🤝 Contributing

Found a bug or want to improve the code?
1. Fork the repo
2. Create a branch: `git checkout -b feature/my-feature`
3. Commit: `git commit -m "Add my feature"`
4. Push: `git push origin feature/my-feature`
5. Open a Pull Request

---

## ❓ FAQ

**Q: Can I use a different microcontroller (Arduino Uno, STM32)?**  
A: Yes, but you'll need to adapt the ADC pins (ADC1 vs ADC0 mapping differs). The logic remains the same.

**Q: What if I don't have Wi-Fi in my grow room?**  
A: Set `USE_WIFI = false`. The firmware uses a "fail-safe" photoperiod logic (treats missing NTP as daytime). Lights stay on during the programmed window.

**Q: Can I grow something other than chickpea?**  
A: Absolutely! The thresholds are species-specific. Update the temperature and RH bands (lines 46–54) for your plant. Research your crop's optimal growing conditions.

**Q: How long will the system run autonomously?**  
A: As long as power is stable. The firmware was tested for 7-day continuous operation. No moving parts = high reliability.

**Q: Can I use this on a 9 V battery?**  
A: Not recommended. Relays require stable 5 V coil power. Use a USB power bank for temporary mobile setup (not recommended for grow boxes).

---

## 📞 Support & Questions

- **GitHub Issues:** Open an issue for bugs or feature requests
- **Project Report:** See PDF for detailed tuning and background
- **Troubleshooting:** See [Troubleshooting](#-troubleshooting) section above

---

## 🎉 Acknowledgments

- **Adafruit** for DHT and LCD libraries
- **Espressif** for ESP32 ecosystem
- **Plant science literature** on *Cicer arietinum* optimal growth conditions
- **Contributors** and testers who helped refine the thresholds

---

**Built with ❤️ for indoor farming enthusiasts and AI hobbyists.**

*Last updated: August 28, 2025*

---

### 🚀 **Next Steps**
1. ✅ Gather hardware (see BOM)
2. ✅ Wire your system (see wiring table)
3. ✅ Run calibration sketch
4. ✅ Upload main controller
5. ✅ Monitor Serial output
6. ✅ Enjoy automated plant farming!

**Happy growing! 🌱**
