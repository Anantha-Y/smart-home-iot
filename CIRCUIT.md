# Circuit & Wiring Guide

## Bill of Materials

| Component | Qty | Notes |
|---|---|---|
| Arduino UNO (ATmega328P) | 1 | 5 V logic |
| ESP8266 NodeMCU v1.0 | 1 | 3.3 V logic, CP2102 USB-serial |
| DHT11 Sensor | 1 | ±2 °C, ±5 %RH |
| LDR (Light Dependent Resistor) | 1 | 10 kΩ dark, ~1 kΩ bright |
| SSD1306 OLED 0.96" (128×64 I²C) | 1 | Address 0x3C |
| 1-channel relay module (5 V coil) | 4 | Active HIGH |
| 10 kΩ resistor | 1 | DHT11 pull-up |
| 10 kΩ resistor | 1 | LDR voltage divider |
| 1 kΩ resistor | 1 | Voltage divider R1 (Arduino TX → ESP) |
| 2 kΩ resistor | 1 | Voltage divider R2 (to GND) |
| Breadboard + jumper wires | — | |

**Estimated total cost: ~₹1,080 (< 15 USD)**

---

## Pin Connections

### Arduino UNO

| Arduino Pin | Connects To | Notes |
|---|---|---|
| 5V | Relay VCC × 4, DHT11 VCC | Power rail |
| GND | Common ground | |
| D4 | DHT11 Data (+ 10 kΩ pull-up to 5V) | |
| A0 | LDR-resistor divider mid-point | 10 kΩ + LDR to GND |
| D8 | Relay 1 IN (Kitchen) | |
| D9 | Relay 2 IN (Hall) | |
| D10 | Relay 3 IN (Bedroom) | |
| D11 | Relay 4 IN (Outdoor) | |
| A4 (SDA) | OLED SDA | I²C |
| A5 (SCL) | OLED SCL | I²C |
| A2 | → Voltage divider R1 → ESP8266 D5 | 5V TX → 3.3V |
| A3 | ← ESP8266 D7 | 3.3V safe (Arduino inputs are 5V tolerant) |

### Voltage Divider (5V → 3.33V)

```
Arduino A2 (TX 5V)
        │
       [1kΩ]   ← R1
        │
        ├─────────────────── ESP8266 D5 (RX)
        │
       [2kΩ]   ← R2
        │
       GND
```

Output = 5 V × (2 kΩ / (1 kΩ + 2 kΩ)) = **3.33 V** ✓

### ESP8266 NodeMCU

| ESP8266 Pin | Connects To | Notes |
|---|---|---|
| 3.3V | — | Powered from USB via onboard regulator |
| GND | Common ground | |
| D5 | Voltage divider output (from Arduino A2) | RX from Arduino |
| D7 | Arduino A3 | TX to Arduino (3.3V direct, safe) |
| USB | PC or 5V USB power | Programming + serial monitor |

---

## Relay Wiring (per relay)

Each relay switches 230 V AC load (lights / fans). Wire the relay NO
(Normally Open) + COM terminals in series with the live wire of the load.

> ⚠️ **Safety**: 230 V AC is lethal. Ensure all mains-voltage wiring is
> inside an insulated enclosure. Use fused connections and earth bonding.
> For a student prototype, wire only low-voltage (5 V DC LEDs) to the relay
> load terminals during testing.

---

## Power Supply

| Rail | Source | Notes |
|---|---|---|
| Arduino 5V | USB from PC or 5V adapter | Powers Arduino + relays + sensors |
| ESP8266 3.3V | USB from same PC or separate 5V adapter via NodeMCU regulator | Keep USB grounds common |

Both Arduinos must share a **common GND** even if powered separately.

---

## OLED (SSD1306) — I²C Wiring

| OLED Pin | Arduino Pin |
|---|---|
| GND | GND |
| VDD (VCC) | 3.3 V or 5 V (module has regulator) |
| SCK | A5 |
| SDA | A4 |

---

## Schematic Diagram

See `docs/figures/fig_arch.png` for the logical layer view and
`docs/figures/fig_deploy.png` for the full system deployment diagram.

A KiCad schematic file (`docs/schematic/smart_home.kicad_sch`) is
planned for a future release.
