/**
 * Smart Home Automation System — ESP8266 NodeMCU Firmware
 *
 * Role:    Wi-Fi gateway, MQTT/TLS broker client, local HTTP server
 * Board:   ESP8266 NodeMCU (3.3V, 80 MHz)
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
 *   RX from Arduino  → D5 (via voltage divider: Arduino A2 5V → 3.33V)
 *   TX to Arduino    → D7
 *
 * SECURITY NOTE:
 *   Fill in WIFI_SSID, WIFI_PASSWORD and MQTT_PASSWORD in secrets.h
 *   DO NOT commit secrets.h to a public repository.
 */

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <PubSubClient.h>
#include <WiFiClientSecure.h>
#include <SoftwareSerial.h>
#include <ArduinoJson.h>
#include "secrets.h"       // ← create from secrets.h.example

// ─── MQTT broker (HiveMQ Cloud) ──────────────────────────────────────────────
#define MQTT_BROKER   "YOUR_HIVEMQ_BROKER.hivemq.cloud"  // replace
#define MQTT_PORT     8883
#define MQTT_USER     "smarthome_user"                    // replace
// MQTT_PASSWORD defined in secrets.h

// ─── MQTT topics ─────────────────────────────────────────────────────────────
#define TOPIC_CMD     "home/cmd"       // subscribe — receive commands
#define TOPIC_STATUS  "home/status"    // publish   — sensor + relay state
#define TOPIC_MODE    "home/mode"      // subscribe — AUTO/MANUAL switch

// ─── Serial (UART to Arduino) ─────────────────────────────────────────────────
#define SOFT_RX D5
#define SOFT_TX D7
#define BAUD_SOFT 1200

SoftwareSerial arduinoSerial(SOFT_RX, SOFT_TX);

// ─── State ───────────────────────────────────────────────────────────────────
float    temperature  = 0.0;
float    humidity     = 0.0;
int      ldrValue     = 0;
String   lightCond    = "BRIGHT";
bool     relayState[4] = {false, false, false, false};
bool     autoMode     = true;
unsigned long lastMqttPublish = 0;
#define  MQTT_PUBLISH_INTERVAL 5000   // ms

const char* ROOM_LOWER[4] = {"kitchen", "hall", "bedroom", "outdoor"};
const char* ROOM_UPPER[4] = {"KITCHEN", "HALL", "BEDROOM", "OUTDOOR"};

// ─── Serial buffer ───────────────────────────────────────────────────────────
String inputBuffer = "";

// ─── Objects ─────────────────────────────────────────────────────────────────
WiFiClientSecure  wifiClient;
PubSubClient      mqttClient(wifiClient);
ESP8266WebServer  httpServer(80);

// ─── Wi-Fi connect ───────────────────────────────────────────────────────────
void connectWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print(F("Connecting to Wi-Fi"));
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 30) {
    delay(500);
    Serial.print('.');
    attempts++;
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println();
    Serial.print(F("IP: "));
    Serial.println(WiFi.localIP());
  } else {
    Serial.println(F("\nWi-Fi failed — running offline"));
  }
}

// ─── MQTT callback ───────────────────────────────────────────────────────────
void mqttCallback(char* topic, byte* payload, unsigned int length) {
  String msg = "";
  for (unsigned int i = 0; i < length; i++) msg += (char)payload[i];
  msg.trim();
  Serial.print(F("MQTT IN [")); Serial.print(topic); Serial.print(F("]: "));
  Serial.println(msg);

  String topicStr = String(topic);

  if (topicStr == TOPIC_CMD) {
    // Expected format: {"room":"kitchen","state":"ON"}
    StaticJsonDocument<128> doc;
    if (!deserializeJson(doc, msg)) {
      String room  = doc["room"]  | "";
      String state = doc["state"] | "";
      room.toLowerCase();
      // Forward CMD to Arduino
      arduinoSerial.print("CMD,");
      arduinoSerial.print(room);
      arduinoSerial.print(",");
      arduinoSerial.println(state);
    }
  } else if (topicStr == TOPIC_MODE) {
    msg.toUpperCase();
    autoMode = (msg == "AUTO");
    arduinoSerial.print("MODE,");
    arduinoSerial.println(msg);
  }
}

// ─── MQTT connect / reconnect ────────────────────────────────────────────────
void ensureMqtt() {
  if (WiFi.status() != WL_CONNECTED) return;
  if (mqttClient.connected()) return;

  Serial.print(F("MQTT connecting..."));
  // TLS: skip certificate verification (acceptable for student project)
  // For production: load CA cert with wifiClient.setTrustAnchors(&cert)
  wifiClient.setInsecure();

  String clientId = "smarthome-" + String(ESP.getChipId(), HEX);
  if (mqttClient.connect(clientId.c_str(), MQTT_USER, MQTT_PASSWORD)) {
    Serial.println(F(" connected"));
    mqttClient.subscribe(TOPIC_CMD);
    mqttClient.subscribe(TOPIC_MODE);
  } else {
    Serial.print(F(" failed, rc="));
    Serial.println(mqttClient.state());
  }
}

// ─── Publish status to MQTT ──────────────────────────────────────────────────
void publishStatus() {
  if (!mqttClient.connected()) return;
  StaticJsonDocument<256> doc;
  doc["temperature"] = temperature;
  doc["humidity"]    = humidity;
  doc["ldr"]         = ldrValue;
  doc["light"]       = lightCond;
  doc["mode"]        = autoMode ? "AUTO" : "MANUAL";
  JsonObject relays  = doc.createNestedObject("relays");
  for (int i = 0; i < 4; i++) {
    relays[ROOM_LOWER[i]] = relayState[i];
  }
  char buf[256];
  serializeJson(doc, buf);
  mqttClient.publish(TOPIC_STATUS, buf);
}

// ─── HTTP server handlers ─────────────────────────────────────────────────────
// GET /  → simple HTML dashboard
void handleRoot() {
  String html = R"rawliteral(
<!DOCTYPE html><html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>Smart Home Dashboard</title>
  <style>
    body{font-family:sans-serif;margin:0;padding:20px;background:#f0f4ff}
    h1{color:#001F5B}
    .card{background:#fff;border-radius:12px;padding:20px;margin-bottom:16px;box-shadow:0 2px 8px rgba(0,0,0,.1)}
    .sensor{display:flex;gap:24px;flex-wrap:wrap}
    .stat{background:#001F5B;color:#fff;border-radius:10px;padding:16px 24px;text-align:center}
    .stat span{font-size:2em;font-weight:700;color:#C8993A}
    .relay-grid{display:grid;grid-template-columns:repeat(2,1fr);gap:12px}
    .relay-btn{padding:14px;border:none;border-radius:10px;font-size:1em;font-weight:600;cursor:pointer;transition:.2s}
    .relay-on {background:#C8993A;color:#fff}
    .relay-off{background:#e0e0e0;color:#333}
    .mode-btn{padding:10px 24px;border:none;border-radius:8px;cursor:pointer;font-weight:600;margin:4px}
    #autoBtn {background:#001F5B;color:#fff}
    #manBtn  {background:#e0e0e0;color:#333}
  </style>
</head>
<body>
  <h1>🏠 Smart Home Dashboard</h1>
  <div class="card">
    <h2>Sensors</h2>
    <div class="sensor" id="sensors">Loading…</div>
  </div>
  <div class="card">
    <h2>Mode &nbsp;
      <button class="mode-btn" id="autoBtn" onclick="setMode('AUTO')">AUTO</button>
      <button class="mode-btn" id="manBtn"  onclick="setMode('MANUAL')">MANUAL</button>
    </h2>
  </div>
  <div class="card">
    <h2>Relays</h2>
    <div class="relay-grid" id="relays">Loading…</div>
  </div>
  <script>
    function fetchData(){
      fetch('/data').then(r=>r.json()).then(d=>{
        document.getElementById('sensors').innerHTML=
          '<div class="stat"><div>Temp</div><span>'+d.temperature.toFixed(1)+'°C</span></div>'+
          '<div class="stat"><div>Humidity</div><span>'+d.humidity.toFixed(1)+'%</span></div>'+
          '<div class="stat"><div>Light</div><span>'+d.light+'</span></div>'+
          '<div class="stat"><div>Mode</div><span>'+d.mode+'</span></div>';
        let html='';
        ['kitchen','hall','bedroom','outdoor'].forEach(r=>{
          let on=d.relays[r];
          html+='<button class="relay-btn '+(on?'relay-on':'relay-off')+'" onclick="toggle(\''+r+'\','+(!on)+')">'
               +r.charAt(0).toUpperCase()+r.slice(1)+' '+(on?'ON':'OFF')+'</button>';
        });
        document.getElementById('relays').innerHTML=html;
      });
    }
    function toggle(room,state){
      fetch('/relay?room='+room+'&state='+(state?'ON':'OFF'));
      setTimeout(fetchData,400);
    }
    function setMode(m){fetch('/mode?value='+m);setTimeout(fetchData,400);}
    fetchData(); setInterval(fetchData,3000);
  </script>
</body></html>
)rawliteral";
  httpServer.send(200, "text/html", html);
}

// GET /data  → JSON sensor + relay snapshot
void handleData() {
  StaticJsonDocument<256> doc;
  doc["temperature"] = temperature;
  doc["humidity"]    = humidity;
  doc["ldr"]         = ldrValue;
  doc["light"]       = lightCond;
  doc["mode"]        = autoMode ? "AUTO" : "MANUAL";
  JsonObject relays  = doc.createNestedObject("relays");
  for (int i = 0; i < 4; i++) relays[ROOM_LOWER[i]] = relayState[i];
  String out;
  serializeJson(doc, out);
  httpServer.send(200, "application/json", out);
}

// GET /relay?room=kitchen&state=ON
void handleRelay() {
  if (!httpServer.hasArg("room") || !httpServer.hasArg("state")) {
    httpServer.send(400, "text/plain", "Missing room or state");
    return;
  }
  String room  = httpServer.arg("room");
  String state = httpServer.arg("state");
  room.toUpperCase();
  arduinoSerial.print("CMD,");
  arduinoSerial.print(room);
  arduinoSerial.print(",");
  arduinoSerial.println(state);
  httpServer.send(200, "text/plain", "OK");
}

// GET /mode?value=AUTO|MANUAL
void handleMode() {
  if (!httpServer.hasArg("value")) {
    httpServer.send(400, "text/plain", "Missing value");
    return;
  }
  String mode = httpServer.arg("value");
  mode.toUpperCase();
  autoMode = (mode == "AUTO");
  arduinoSerial.print("MODE,");
  arduinoSerial.println(mode);
  httpServer.send(200, "text/plain", "OK");
}

// GET /room?name=kitchen  → single relay JSON
void handleRoom() {
  if (!httpServer.hasArg("name")) {
    httpServer.send(400, "text/plain", "Missing name");
    return;
  }
  String name = httpServer.arg("name");
  name.toLowerCase();
  for (int i = 0; i < 4; i++) {
    if (name == String(ROOM_LOWER[i])) {
      String out = "{\"room\":\"" + name + "\",\"state\":" + (relayState[i] ? "true" : "false") + "}";
      httpServer.send(200, "application/json", out);
      return;
    }
  }
  httpServer.send(404, "text/plain", "Unknown room");
}

// ─── Process line from Arduino ────────────────────────────────────────────────
void processArduinoLine(String line) {
  line.trim();
  if (line.length() == 0) return;
  Serial.print(F("ARD: ")); Serial.println(line);

  if (line.startsWith("SENSOR,")) {
    // SENSOR,30.0,62.0,95,DARK
    int p1 = 7;
    int p2 = line.indexOf(',', p1);
    int p3 = line.indexOf(',', p2 + 1);
    int p4 = line.indexOf(',', p3 + 1);
    if (p2 < 0 || p3 < 0 || p4 < 0) return;
    temperature = line.substring(p1, p2).toFloat();
    humidity    = line.substring(p2 + 1, p3).toFloat();
    ldrValue    = line.substring(p3 + 1, p4).toInt();
    lightCond   = line.substring(p4 + 1);

  } else if (line.startsWith("STATE,")) {
    // STATE,kitchen,ON
    int c1 = line.indexOf(',');
    int c2 = line.indexOf(',', c1 + 1);
    if (c1 < 0 || c2 < 0) return;
    String room  = line.substring(c1 + 1, c2);
    String state = line.substring(c2 + 1);
    room.toLowerCase();
    for (int i = 0; i < 4; i++) {
      if (room == String(ROOM_LOWER[i])) {
        relayState[i] = (state == "ON");
        break;
      }
    }
  }
}

// ─── Setup ───────────────────────────────────────────────────────────────────
void setup() {
  Serial.begin(115200);
  arduinoSerial.begin(BAUD_SOFT);

  connectWiFi();

  mqttClient.setServer(MQTT_BROKER, MQTT_PORT);
  mqttClient.setCallback(mqttCallback);
  mqttClient.setBufferSize(512);

  httpServer.on("/",      handleRoot);
  httpServer.on("/data",  handleData);
  httpServer.on("/relay", handleRelay);
  httpServer.on("/mode",  handleMode);
  httpServer.on("/room",  handleRoom);
  httpServer.begin();

  Serial.println(F("ESP8266 ready"));
  Serial.print(F("Web dashboard: http://"));
  Serial.println(WiFi.localIP());
}

// ─── Main loop ────────────────────────────────────────────────────────────────
void loop() {
  httpServer.handleClient();
  ensureMqtt();
  if (mqttClient.connected()) mqttClient.loop();

  // Read lines from Arduino
  while (arduinoSerial.available()) {
    char c = (char)arduinoSerial.read();
    if (c == '\n') {
      processArduinoLine(inputBuffer);
      inputBuffer = "";
    } else if (c != '\r') {
      inputBuffer += c;
    }
  }

  // Periodic MQTT publish
  if (millis() - lastMqttPublish >= MQTT_PUBLISH_INTERVAL) {
    publishStatus();
    lastMqttPublish = millis();
  }
}
