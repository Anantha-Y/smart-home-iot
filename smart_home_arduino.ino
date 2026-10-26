/**
 * Smart Home Automation System — Arduino UNO Firmware
 *
 * Role:    Sensor acquisition, relay control, OLED display
 * Board:   Arduino UNO (ATmega328P, 5V)
 *
 * Authors: Anantha Vishwa Priya Y  (RA2411047020009)
 *          Aditya Nawle            (RA2411047020021)
 *
 * Course:  21AIE221T – Internet of Things Architecture and Protocol
 * Faculty: Dr. Judy Flavia, Dept. of Artificial Intelligence
 * Inst.:   SRM Institute of Science and Technology, Ramapuram
 * Year:    2024–2025
 *
 * Hardware connections:
 *   DHT11 data       → D4
 *   LDR (analog)     → A0
 *   Relay 1 (Kitchen)→ D8
 *   Relay 2 (Hall)   → D9
 *   Relay 3 (Bedroom)→ D10
 *   Relay 4 (Outdoor)→ D11
 *   OLED SDA         → A4 (SDA)
 *   OLED SCL         → A5 (SCL)
 *   TX to ESP8266    → A2  (via 1kΩ/2kΩ voltage divider → 3.3V)
 *   RX from ESP8266  → A3
 */

#include <SoftwareSerial.h>
#include <DHT.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// ─── Pin definitions ─────────────────────────────────────────────────────────
#define DHT_PIN       4
#define DHT_TYPE      DHT11
#define LDR_PIN       A0
#define RELAY_KITCHEN 8
#define RELAY_HALL    9
#define RELAY_BEDROOM 10
#define RELAY_OUTDOOR 11
#define SOFT_TX       A2    // → ESP8266 D5 via voltage divider
#define SOFT_RX       A3    // ← ESP8266 D7

// ─── Constants ───────────────────────────────────────────────────────────────
#define SCREEN_WIDTH    128
#define SCREEN_HEIGHT    64
#define OLED_RESET       -1
#define OLED_ADDRESS   0x3C
#define LDR_THRESHOLD   200   // below = dark → AUTO ON
#define LDR_HYSTERESIS   10   // prevents flutter
#define SENSOR_INTERVAL 2000  // ms between sensor reads
#define BAUD_SOFT       1200  // SoftwareSerial baud rate

// ─── Objects ─────────────────────────────────────────────────────────────────
SoftwareSerial espSerial(SOFT_RX, SOFT_TX);  // RX, TX
DHT dht(DHT_PIN, DHT_TYPE);
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// ─── State ───────────────────────────────────────────────────────────────────
bool relayState[4] = {false, false, false, false};
const char* ROOM_NAMES[4] = {"KITCHEN", "HALL", "BEDROOM", "OUTDOOR"};
const char* ROOM_LOWER[4] = {"kitchen", "hall",  "bedroom", "outdoor"};
const uint8_t RELAY_PINS[4] = {RELAY_KITCHEN, RELAY_HALL, RELAY_BEDROOM, RELAY_OUTDOOR};

bool autoMode        = true;
bool lastAutoState   = false;   // for hysteresis
float temperature    = 0.0;
float humidity       = 0.0;
int   ldrValue       = 0;
unsigned long lastSensorTime = 0;

// Serial input buffer
String inputBuffer = "";

// ─── Setup ───────────────────────────────────────────────────────────────────
void setup() {
  Serial.begin(9600);
  espSerial.begin(BAUD_SOFT);

  // Relay pins — active HIGH, default OFF
  for (int i = 0; i < 4; i++) {
    pinMode(RELAY_PINS[i], OUTPUT);
    digitalWrite(RELAY_PINS[i], LOW);
  }

  dht.begin();

  // OLED
  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDRESS)) {
    Serial.println(F("SSD1306 init failed"));
    for (;;);
  }
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println(F("Smart Home v1.0"));
  display.println(F("Initialising..."));
  display.display();
  delay(1500);

  Serial.println(F("Arduino ready"));
}

// ─── Main loop ────────────────────────────────────────────────────────────────
void loop() {
  // 1. Read sensors at interval
  if (millis() - lastSensorTime >= SENSOR_INTERVAL) {
    readSensors();
    lastSensorTime = millis();

    // AUTO mode: drive all relays from LDR
    if (autoMode) {
      bool dark = (ldrValue < (lastAutoState
                               ? LDR_THRESHOLD + LDR_HYSTERESIS
                               : LDR_THRESHOLD - LDR_HYSTERESIS));
      if (dark != lastAutoState) {
        lastAutoState = dark;
        for (int i = 0; i < 4; i++) {
          setRelay(i, dark);
        }
      }
    }

    // 2. Publish sensor data to ESP8266
    sendSensorData();

    // 3. Update OLED
    updateOLED();
  }

  // 4. Process commands from ESP8266
  while (espSerial.available()) {
    char c = (char)espSerial.read();
    if (c == '\n') {
      processCommand(inputBuffer);
      inputBuffer = "";
    } else if (c != '\r') {
      inputBuffer += c;
    }
  }
}

// ─── Sensor read ─────────────────────────────────────────────────────────────
void readSensors() {
  float t = dht.readTemperature();
  float h = dht.readHumidity();
  if (!isnan(t)) temperature = t;
  if (!isnan(h)) humidity    = h;
  ldrValue = analogRead(LDR_PIN);
}

// ─── Relay control ───────────────────────────────────────────────────────────
void setRelay(int idx, bool on) {
  if (idx < 0 || idx >= 4) return;
  relayState[idx] = on;
  digitalWrite(RELAY_PINS[idx], on ? HIGH : LOW);

  // Echo STATE back to ESP8266
  // Format: STATE,<room>,<ON|OFF>
  espSerial.print("STATE,");
  espSerial.print(ROOM_LOWER[idx]);
  espSerial.print(",");
  espSerial.println(on ? "ON" : "OFF");
}

// ─── Send sensor data to ESP8266 ─────────────────────────────────────────────
// Format: SENSOR,<temp>,<humidity>,<ldr>,<DARK|BRIGHT>
void sendSensorData() {
  espSerial.print("SENSOR,");
  espSerial.print(temperature, 1);
  espSerial.print(",");
  espSerial.print(humidity, 1);
  espSerial.print(",");
  espSerial.print(ldrValue);
  espSerial.print(",");
  espSerial.println(ldrValue < LDR_THRESHOLD ? "DARK" : "BRIGHT");
}

// ─── Process command from ESP8266 ────────────────────────────────────────────
// Formats:
//   CMD,<ROOM>,<ON|OFF>   — toggle individual relay
//   MODE,<AUTO|MANUAL>    — switch operating mode
void processCommand(String cmd) {
  cmd.trim();
  if (cmd.length() == 0) return;

  Serial.print(F("CMD: ")); Serial.println(cmd);

  if (cmd.startsWith("CMD,")) {
    // CMD,KITCHEN,ON
    int c1 = cmd.indexOf(',');
    int c2 = cmd.indexOf(',', c1 + 1);
    if (c1 < 0 || c2 < 0) return;

    String room   = cmd.substring(c1 + 1, c2);
    String action = cmd.substring(c2 + 1);
    room.toUpperCase();
    action.trim();

    for (int i = 0; i < 4; i++) {
      if (room == String(ROOM_NAMES[i])) {
        setRelay(i, action == "ON");
        break;
      }
    }

  } else if (cmd.startsWith("MODE,")) {
    String mode = cmd.substring(5);
    mode.trim();
    if (mode == "AUTO") {
      autoMode = true;
      Serial.println(F("Switched to AUTO mode"));
    } else if (mode == "MANUAL") {
      autoMode = false;
      Serial.println(F("Switched to MANUAL mode"));
    }
  }
}

// ─── OLED update ─────────────────────────────────────────────────────────────
void updateOLED() {
  display.clearDisplay();
  display.setTextSize(1);

  // Line 0: mode + light condition
  display.setCursor(0, 0);
  display.print(autoMode ? F("AUTO ") : F("MANU "));
  display.println(ldrValue < LDR_THRESHOLD ? F("DARK") : F("BRIGHT"));

  // Line 1: temperature & humidity
  display.setCursor(0, 10);
  display.print(temperature, 1);
  display.print(F("\xF7""C  "));
  display.print(humidity, 1);
  display.println(F("%RH"));

  // Line 2: LDR value
  display.setCursor(0, 20);
  display.print(F("LDR: "));
  display.println(ldrValue);

  // Lines 3-6: relay states
  for (int i = 0; i < 4; i++) {
    display.setCursor(0, 30 + i * 8);
    display.print(ROOM_NAMES[i]);
    display.print(F(": "));
    display.println(relayState[i] ? F("ON ") : F("OFF"));
  }

  display.display();
}
