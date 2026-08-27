# Complete point-to-point wiring table

No diagram. Every conductor. “Star GND” = one common node: ESP32 GND + relay GND + 12 V adapter − + any 5 V charger −.

## A. ESP32 DevKit V1 (30-pin) — power

| From | Pin / lead | To | Pin / lead | Notes |
|---|---|---|---|---|
| Laptop / USB 2 A brick | USB-A 5 V | ESP32 | Micro-USB | Logic only. Do **not** hang the pump on this. |
| ESP32 | `3V3` | DHT11, soil module, LDR module | `VCC` | Keeps analog ≤ 3.3 V |
| ESP32 | `5V` | LCD backpack `VCC` and relay `VCC` | 5 V | Relay **coils** only |
| ESP32 | `GND` | Star GND | — | Mandatory |

## B. Sensors

### DHT11 (temperature + humidity, one chip)

| DHT11 | Goes to | ESP32 |
|---|---|---|
| `VCC` | red | `3V3` |
| `DATA` / `OUT` | yellow | **GPIO 4** |
| `GND` | black | `GND` |
| unused NC (4-pin raw sensor) | — | leave open |

### Soil moisture module (AO used, DO ignored)

| Module | Goes to | ESP32 |
|---|---|---|
| `VCC` | red | `3V3`  (not 5 V) |
| `AO` / `A0` | green | **GPIO 34** (ADC1) |
| `GND` | black | `GND` |
| `DO` / `D0` | — | **not connected** |

Calibrated: dry (air) ADC = **4095**, wet ADC ≈ **1200**.

### LM393 LDR module (AO used)

| Module | Goes to | ESP32 |
|---|---|---|
| `VCC` | red | `3V3` |
| `AO` / `A0` | orange | **GPIO 35** (ADC1) |
| `GND` | black | `GND` |
| `DO` | — | not connected |
| 18 kΩ resistor | — | **not used** (module already has a divider) |

Dark trip in firmware: ADC ≥ **2900**.

### 16×2 LCD + I²C backpack

| Backpack | Goes to | ESP32 |
|---|---|---|
| `VCC` | red | `5V` (or `3V3` if your backpack is 3.3 V only) |
| `GND` | black | `GND` |
| `SDA` | blue | **GPIO 21** |
| `SCL` | purple | **GPIO 22** |

I²C address `0x27` (try `0x3F` if backlight-only).

## C. 4-channel relay HW-316 — logic header

JD-VCC jumper **ON**. Active-LOW inputs.

| Relay header | Goes to | ESP32 | Load this channel will switch |
|---|---|---|---|
| `GND` | black | `GND` | — |
| `IN1` | white | **GPIO 26** | Humidifier (5 V, later) |
| `IN2` | white | **GPIO 27** | Water pump (5 V) |
| `IN3` | white | **GPIO 25** | Ventilation fan (12 V) |
| `IN4` | white | **GPIO 33** | LED strip (match strip voltage) |
| `VCC` | red | ESP32 `5V` | coil supply |

## D. Actuator (load) side — COM–NO only, never NC

Each channel is a switch in the **positive** lead. Negative of the load goes to that load’s own supply negative, which is also tied to star GND.

### Channel 3 — 12 V OSL-8021 fan  (working)

| From | To |
|---|---|
| 12 V adapter **+** | Relay ch.3 **COM** |
| Relay ch.3 **NO** | Fan **+** (red) |
| Fan **−** (black) | 12 V adapter **−** |
| 12 V adapter **−** | Star GND |

### Channel 4 — LED strip  (working, reseat if intermittent)

| If the strip is **12 V** | |
|---|---|
| 12 V adapter **+** | Relay ch.4 **COM** |
| Relay ch.4 **NO** | Strip **+** (usually red / 12 V pad) |
| Strip **−** (GND pad) | 12 V adapter **−** / star GND |

| If the strip is **5 V** | |
|---|---|
| Dedicated 5 V 2 A charger **+** | Relay ch.4 **COM** |
| Relay ch.4 **NO** | Strip **+** |
| Strip **−** | That charger **−** / star GND |

Do **not** feed the strip from the ESP32 `5V` pin.

### Channel 2 — 5 V mini pump  (command works; motor needs its own 5 V brick)

| From | To |
|---|---|
| Phone-charger 5 V 2 A **+** | Relay ch.2 **COM** |
| Relay ch.2 **NO** | Pump **+** |
| Pump **−** | Charger **−** |
| Charger **−** | Star GND |

**Wrong (this is what crashed the LCD):** pump **+** taken from ESP32 `5V` / laptop USB.

### Channel 1 — USB humidifier  (same as pump, pending a cut USB lead)

| From | To |
|---|---|
| Same 5 V 2 A charger **+** | Relay ch.1 **COM** |
| Relay ch.1 **NO** | USB cable **red** (atomizer +) |
| USB cable **black** | Charger **−** / star GND |

No spare USB cable ⇒ leave screw terminals empty. The IN1 click test already proved the channel.

## E. What must never be connected

| Don’t | Why |
|---|---|
| 12 V into any ESP32 pin | Destroys the chip |
| Soil / LDR `VCC` to 5 V | AO can exceed 3.3 V ADC |
| Pump or strip on ESP32 `5V` | Brownout (you already measured this) |
| Motors on the breadboard | Current / heat |
| NC terminal of a relay | Inverts on/off |
