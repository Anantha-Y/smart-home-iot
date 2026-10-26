# 🏠 Smart Home Automation System

> IoT-based smart home controller using Arduino UNO + ESP8266 NodeMCU  
> with MQTT over TLS, local HTTP dashboard, and automatic light control.

---

## Authors

| Name | Register No. |
|---|---|
| **Anantha Vishwa Priya Y** | RA2411047020009 |
| **Aditya Nawle** | RA2411047020021 |

**Faculty Guide:** Dr. Judy Flavia, Assistant Professor, Department of Artificial Intelligence  
**Course:** 21AIE221T – Internet of Things Architecture and Protocol  
**Institution:** SRM Institute of Science and Technology, Ramapuram, Chennai  
**Academic Year:** 2024–2025 · AI-A · 3rd Year · 5th Semester

---

## Overview

This project implements a low-cost (~₹1,080 / < 15 USD) smart home
automation system using a **dual-microcontroller** architecture:

| Microcontroller | Role | Voltage |
|---|---|---|
| **Arduino UNO** (ATmega328P) | Sensor acquisition, relay control, OLED display | 5 V |
| **ESP8266 NodeMCU** | Wi-Fi gateway, MQTT/TLS broker, HTTP web server | 3.3 V |

The two boards communicate over **SoftwareSerial UART at 1200 baud** using a
CSV protocol. A passive **voltage divider (1 kΩ + 2 kΩ)** safely steps the
Arduino's 5 V TX down to 3.33 V for the ESP8266 RX line.

---

## Features

- **AUTO mode** — LDR threshold (< 200 ADC) triggers all relays with
  hysteresis (±10) to prevent flutter
- **MANUAL mode** — individual room control via web dashboard or MQTT
- **MQTT over TLS** (HiveMQ Cloud, port 8883) — remote control from anywhere
- **Local HTTP server** (port 80) — dashboard accessible on LAN without
  internet
- **SSD1306 OLED display** — live temperature, humidity, light condition
  and relay states
- **DHT11 sensor** — temperature (±2 °C) and humidity (±5 %RH) monitoring

---

## Repository Structure

```
smart-home-iot/
├── src/
│   ├── arduino/
│   │   ├── smart_home_arduino.ino   ← Arduino UNO firmware
│   │   └── libraries.txt            ← required Arduino libraries
│   └── esp8266/
│       ├── smart_home_esp8266.ino   ← ESP8266 NodeMCU firmware
│       ├── secrets.h.example        ← copy to secrets.h and fill in
│       └── libraries.txt            ← required libraries
├── scripts/
│   └── generate_diagrams.py         ← matplotlib diagram generator
├── docs/
│   ├── CIRCUIT.md                   ← wiring + BOM
│   ├── MQTT_SETUP.md                ← HiveMQ Cloud configuration
│   └── figures/                     ← generated diagram PNGs
├── .gitignore
├── LICENSE
└── README.md
```

---

## Quick Start

### 1. Clone

```bash
git clone https://github.com/YOUR_USERNAME/smart-home-iot.git
cd smart-home-iot
```

### 2. Install Arduino IDE libraries

See `src/arduino/libraries.txt` and `src/esp8266/libraries.txt`.  
Install via **Arduino IDE → Sketch → Include Library → Manage Libraries**.

For ESP8266 board support, add this URL to
**File → Preferences → Additional Board Manager URLs**:

```
https://arduino.esp8266.com/stable/package_esp8266com_index.json
```

Then install **ESP8266 by ESP8266 Community** from Board Manager.

### 3. Configure secrets

```bash
cp src/esp8266/secrets.h.example src/esp8266/secrets.h
# Edit secrets.h with your Wi-Fi SSID, password, and HiveMQ password
```

Update `MQTT_BROKER` and `MQTT_USER` in `smart_home_esp8266.ino`.

See [docs/MQTT_SETUP.md](docs/MQTT_SETUP.md) for HiveMQ Cloud setup.

### 4. Flash firmware

1. Open `src/arduino/smart_home_arduino.ino` → select **Arduino UNO** → Upload
2. Open `src/esp8266/smart_home_esp8266.ino` → select **NodeMCU 1.0
   (ESP-12E Module)** → Upload

### 5. Wire the hardware

See [docs/CIRCUIT.md](docs/CIRCUIT.md) for full pin-out and BOM.

### 6. Access the dashboard

After booting, open the Serial Monitor on the ESP8266 at 115200 baud.
It prints the local IP address, e.g.:

```
IP: 192.168.1.42
Web dashboard: http://192.168.1.42
```

Open that URL in any browser on the same Wi-Fi network.

---

## Serial CSV Protocol

Communication between Arduino and ESP8266 is over SoftwareSerial at 1200 baud,
newline-terminated ASCII:

| Message | Direction | Format | Example |
|---|---|---|---|
| CMD | ESP → Arduino | `CMD,<ROOM>,<ON\|OFF>` | `CMD,KITCHEN,ON` |
| STATE | Arduino → ESP | `STATE,<room>,<ON\|OFF>` | `STATE,kitchen,ON` |
| SENSOR | Arduino → ESP | `SENSOR,<T>,<H>,<LDR>,<COND>` | `SENSOR,30.0,62.0,95,DARK` |
| MODE | ESP → Arduino | `MODE,<AUTO\|MANUAL>` | `MODE,MANUAL` |

---

## System Architecture

```
┌─────────────────────────────────────────────────────────┐
│  Application Layer   Web Dashboard / Mobile MQTT App    │
├─────────────────────────────────────────────────────────┤
│  Processing Layer    ESP8266 NodeMCU (MQTT/TLS + HTTP)  │
├─────────────────────────────────────────────────────────┤
│  Network Layer       Wi-Fi 802.11 b/g/n + HiveMQ Cloud  │
├─────────────────────────────────────────────────────────┤
│  Perception Layer    Arduino UNO (DHT11, LDR, Relays)   │
└─────────────────────────────────────────────────────────┘
```

---

## Results

| Metric | Value |
|---|---|
| LAN HTTP command latency (mean) | 52 ms |
| MQTT (Wi-Fi) latency (mean) | 185 ms |
| MQTT (4G mobile) latency (mean) | 295 ms |
| DHT11 temperature max error | 1.9 °C (spec ±2 °C) ✓ |
| DHT11 humidity max error | 4.8 %RH (spec ±5 %) ✓ |
| AUTO mode false triggers (24 h) | 0 |
| Total system cost | ~₹1,080 (< 15 USD) |

---

## Generate Diagrams

```bash
pip install matplotlib numpy
python scripts/generate_diagrams.py
# → docs/figures/fig_arch.png, fig_flow.png, fig_userflow.png,
#              fig_deploy.png, fig_opview.png, fig_serial.png
```

---

## Security Notes

- **Wi-Fi credentials** and **MQTT password** are stored in `secrets.h`
  which is listed in `.gitignore`. Never commit it.
- TLS is enabled (port 8883). The firmware uses `setInsecure()` for
  simplicity; replace with a trust anchor for production.

---

## License

MIT License — see [LICENSE](LICENSE).

---

## References

1. Hunkeler, U., et al. "MQTT-S — A Publish/Subscribe Protocol for Wireless Sensor Networks." *COMSWARE 2008.*
2. Espressif Systems. *ESP8266 Technical Reference*, v1.7, 2020.
3. Microchip Technology. *ATmega328P Datasheet*, 2018.
4. Kumar, J. S. & Patel, D. R. "A Survey on Internet of Things." *IJCA*, 2014.
5. Kodali, R. K. et al. "IoT-based Smart Security and Home Automation System." *CSNT 2016.*
