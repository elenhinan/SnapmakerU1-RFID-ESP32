#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <HTTPClient.h>
#include <SPI.h>

#ifndef MFRC522_SPICLOCK
#define MFRC522_SPICLOCK (1000000u) // 1MHz for maximum signal integrity with jumper wires
#endif
#include <MFRC522.h>
#include <ArduinoJson.h>
#include <Adafruit_NeoPixel.h>
#include <Preferences.h>
#include "web_page.h"

// --- NVS Preferences & Config ---
Preferences prefs;
String wifiSsid        = "YOUR_WIFI_SSID";
String wifiPassword    = "YOUR_WIFI_PASSWORD";
String printerIp       = "10.0.2.30"; // Snapmaker U1 IP
int    moonrakerPort   = 7125;
String spoolmanUrl     = "http://10.0.2.30:7912";
unsigned long stageTimeoutMs = 120000;
const char* MDNS_NAME  = "snapmaker_rfid";

// --- Pin Definitions & LED Geometry ---
#define RC522_SS_PIN      22
#define RC522_RST_PIN     4
#define NEOPIXEL_PIN      26
#define NUM_LEDS          7
#define CENTER_LED_INDEX  6

// Outer ring clockwise order (0° is UP):
// 1: 30° (Top-Right)
// 0: 90° (Right)
// 5: 150° (Bottom-Right)
// 4: 210° / -150° (Bottom-Left)
// 3: 270° / -90° (Left)
// 2: 330° / -30° (Top-Left)
const uint8_t RING_CW[6] = {1, 0, 5, 4, 3, 2};

// --- Hardware & Web Instances ---
MFRC522 rfid(RC522_SS_PIN, RC522_RST_PIN);
Adafruit_NeoPixel pixels(NUM_LEDS, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);
WebServer server(80);

// Sensor names in Snapmaker U1 / Klipper (URL query with %20 vs JSON keys with spaces)
const char* SENSOR_QUERY_NAMES[4] = {
  "filament_motion_sensor%20e0_filament",
  "filament_motion_sensor%20e1_filament",
  "filament_motion_sensor%20e2_filament",
  "filament_motion_sensor%20e3_filament"
};
const char* SENSOR_KEY_NAMES[4] = {
  "filament_motion_sensor e0_filament",
  "filament_motion_sensor e1_filament",
  "filament_motion_sensor e2_filament",
  "filament_motion_sensor e3_filament"
};

const char* FALLBACK_QUERY_NAMES[4] = {
  "filament_switch_sensor%20e0_sensor",
  "filament_switch_sensor%20e1_sensor",
  "filament_switch_sensor%20e2_sensor",
  "filament_switch_sensor%20e3_sensor"
};
const char* FALLBACK_KEY_NAMES[4] = {
  "filament_switch_sensor e0_sensor",
  "filament_switch_sensor e1_sensor",
  "filament_switch_sensor e2_sensor",
  "filament_switch_sensor e3_sensor"
};

// --- State Machine ---
enum SystemState { STATE_IDLE, STATE_ARMED };
SystemState currentState = STATE_IDLE;

String stagedUID = "";
String stagedType = "M1";
int    stagedSpoolId = 0;
String stagedSpoolInfo = "";
String stagedSpoolColor = "";
unsigned long armedTimestamp = 0;

bool previousSensorState[4] = {false, false, false, false};
bool lastMoonrakerOk = false;
bool isApMode = false;

// RFID Diagnostic Variables
uint8_t rfidVersion = 0x00;
bool rfidConnected = false;
unsigned long rfidScanCount = 0;
String lastScannedUid = "";

// LED & Visual Variables
uint8_t animStep = 0;
unsigned long lastAnimUpdate = 0;
bool manualLedMode = false;
uint8_t currentBrightness = 80;

// Non-blocking loop timers
unsigned long lastSensorPoll = 0;
unsigned long lastRfidPoll = 0;

// Scheduled Reboot Flag
bool shouldReboot = false;
unsigned long rebootTimer = 0;

void initRfidHardware() {
  pinMode(RC522_RST_PIN, OUTPUT);
  digitalWrite(RC522_RST_PIN, LOW);
  delay(15);
  digitalWrite(RC522_RST_PIN, HIGH);
  delay(50);

  // Drive GPIO 16 (D4) HIGH in case RST was plugged there
  pinMode(16, OUTPUT);
  digitalWrite(16, HIGH);

  pinMode(RC522_SS_PIN, OUTPUT);
  digitalWrite(RC522_SS_PIN, HIGH);

  SPI.begin(18, 19, 23);
  delay(10);
  rfid.PCD_Init(RC522_SS_PIN, RC522_RST_PIN);
  delay(20);
  rfid.PCD_SetAntennaGain(MFRC522::RxGain_max);
  rfid.PCD_AntennaOn();

  rfidVersion = rfid.PCD_ReadRegister(MFRC522::VersionReg);
  rfidConnected = (rfidVersion != 0x00 && rfidVersion != 0xFF);
}

// --- Helper Functions ---
bool isCardPresent() {
  byte bufferATQA[2];
  byte bufferSize = sizeof(bufferATQA);

  // Reset baud rates and modulation width
  rfid.PCD_WriteRegister(MFRC522::TxModeReg, 0x00);
  rfid.PCD_WriteRegister(MFRC522::RxModeReg, 0x00);
  rfid.PCD_WriteRegister(MFRC522::ModWidthReg, 0x26);

  // Ensure antenna is ON
  rfid.PCD_AntennaOn();

  // Try Wakeup (WUPA) first to detect both newly placed and already resting tags
  MFRC522::StatusCode result = rfid.PICC_WakeupA(bufferATQA, &bufferSize);
  if (result == MFRC522::STATUS_OK || result == MFRC522::STATUS_COLLISION) {
    return true;
  }

  // Fallback to RequestA (REQA)
  bufferSize = sizeof(bufferATQA);
  result = rfid.PICC_RequestA(bufferATQA, &bufferSize);
  if (result == MFRC522::STATUS_OK || result == MFRC522::STATUS_COLLISION) {
    return true;
  }

  // Also try standard library check
  return rfid.PICC_IsNewCardPresent();
}

uint32_t parseHexColor(const String& hexStr) {
  String clean = hexStr;
  if (clean.startsWith("#")) clean = clean.substring(1);
  long number = strtol(clean.c_str(), NULL, 16);
  uint8_t r = (number >> 16) & 0xFF;
  uint8_t g = (number >> 8) & 0xFF;
  uint8_t b = number & 0xFF;
  return pixels.Color(r, g, b);
}

String getPixelHex(uint8_t index) {
  if (index >= NUM_LEDS) return "#000000";
  uint32_t c = pixels.getPixelColor(index);
  uint8_t r = (c >> 16) & 0xFF;
  uint8_t g = (c >> 8) & 0xFF;
  uint8_t b = c & 0xFF;
  char buf[8];
  snprintf(buf, sizeof(buf), "#%02x%02x%02x", r, g, b);
  return String(buf);
}

// --- Preferences Loading & Saving ---
void loadStoredConfig() {
  prefs.begin("snapmaker", false);
  wifiSsid        = prefs.getString("ssid", "YOUR_WIFI_SSID");
  wifiPassword    = prefs.getString("pass", "YOUR_WIFI_PASSWORD");
  printerIp       = prefs.getString("printer_ip", "10.0.2.30");
  if (printerIp == "192.168.1.100" || printerIp.length() == 0) {
    printerIp = "10.0.2.30";
  }
  moonrakerPort   = prefs.getInt("port", 7125);
  spoolmanUrl     = prefs.getString("spoolman_url", "http://10.0.2.2:7912");
  if (spoolmanUrl.length() == 0 || spoolmanUrl.indexOf("192.168.1.100") >= 0 || spoolmanUrl.indexOf("10.0.2.30") >= 0) {
    spoolmanUrl = "http://10.0.2.2:7912";
  }
  int timeoutSec  = prefs.getInt("timeout", 120);
  stageTimeoutMs  = (timeoutSec > 0 ? timeoutSec : 120) * 1000UL;
  currentBrightness = (uint8_t)prefs.getInt("brightness", 80);
  prefs.end();
}

void saveStoredConfig(const String& ssid, const String& pass, const String& ip, int port, const String& spoolman, int timeoutSec) {
  prefs.begin("snapmaker", false);
  if (ssid.length() > 0) prefs.putString("ssid", ssid);
  if (pass.length() > 0) prefs.putString("pass", pass);
  if (ip.length() > 0) prefs.putString("printer_ip", ip);
  if (port > 0) prefs.putInt("port", port);
  prefs.putString("spoolman_url", spoolman);
  if (timeoutSec > 0) prefs.putInt("timeout", timeoutSec);
  prefs.putInt("brightness", currentBrightness);
  prefs.end();
}

// --- LED Animation Functions ---
void setAllPixels(uint32_t color) {
  for (int i = 0; i < NUM_LEDS; i++) {
    pixels.setPixelColor(i, color);
  }
  pixels.show();
}

void flashFeedback(uint32_t color, int count, int delayMs) {
  for (int c = 0; c < count; c++) {
    setAllPixels(color);
    delay(delayMs);
    setAllPixels(pixels.Color(0, 0, 0));
    delay(delayMs);
  }
}

void runChaseTest() {
  for (int round = 0; round < 3; round++) {
    for (int i = 0; i < 6; i++) {
      pixels.clear();
      pixels.setPixelColor(RING_CW[i], pixels.Color(0, 180, 255));
      pixels.show();
      delay(60);
      server.handleClient();
    }
  }
  pixels.clear();
  pixels.show();
}

void runBreatheTest() {
  for (int b = 5; b <= 120; b += 5) {
    pixels.clear();
    pixels.setPixelColor(CENTER_LED_INDEX, pixels.Color(b, b, b));
    pixels.show();
    delay(20);
    server.handleClient();
  }
  for (int b = 120; b >= 5; b -= 5) {
    pixels.clear();
    pixels.setPixelColor(CENTER_LED_INDEX, pixels.Color(b, b, b));
    pixels.show();
    delay(20);
    server.handleClient();
  }
  pixels.clear();
  pixels.show();
}

void updateIdleVisuals() {
  if (millis() - lastAnimUpdate < 40) return;
  lastAnimUpdate = millis();

  if (isApMode) {
    // Hotspot setup mode: Pulse Center LED in Cyan/Blue
    static int apBright = 10;
    static int apDir = 2;
    apBright += apDir;
    if (apBright >= 60 || apBright <= 10) apDir = -apDir;

    pixels.clear();
    pixels.setPixelColor(CENTER_LED_INDEX, pixels.Color(0, apBright, apBright + 20));
    pixels.show();
    return;
  }

  // Normal subtle breathing on center LED (index 6) in soft white
  static int brightness = 10;
  static int dir = 1;
  brightness += dir;
  if (brightness >= 45 || brightness <= 5) dir = -dir;

  pixels.clear();
  pixels.setPixelColor(CENTER_LED_INDEX, pixels.Color(brightness, brightness, brightness));
  pixels.show();
}

void updateArmedVisuals() {
  if (millis() - lastAnimUpdate < 70) return;
  lastAnimUpdate = millis();

  pixels.clear();
  // Center LED (index 6) pulses solid amber
  pixels.setPixelColor(CENTER_LED_INDEX, pixels.Color(120, 50, 0));

  // Outer ring clockwise chase
  uint8_t ringIndex = RING_CW[animStep % 6];
  pixels.setPixelColor(ringIndex, pixels.Color(200, 90, 0));
  pixels.show();

  animStep++;
}

// --- Moonraker & Spoolman Functions ---
void querySpoolmanForTag(const String& uid) {
  stagedSpoolId = 0;
  stagedSpoolInfo = "";
  stagedSpoolColor = "";
  if (spoolmanUrl.length() == 0 || WiFi.status() != WL_CONNECTED) return;

  HTTPClient http;
  http.setTimeout(1500);
  String url = spoolmanUrl;
  if (!url.endsWith("/")) url += "/";
  // Use Spoolman's search API which searches extra.card_uids and tags properly
  url += "api/v1/search?q=" + uid;

  http.begin(url);
  int code = http.GET();
  if (code == 200) {
    String payload = http.getString();
    JsonDocument doc;
    if (!deserializeJson(doc, payload) && doc["spools"].is<JsonArray>() && doc["spools"].size() > 0) {
      JsonObject match = doc["spools"][0];
      JsonObject spool = match["spool"];
      stagedSpoolId = spool["id"].as<int>();
      String vendor = spool["filament"]["vendor"]["name"].as<String>();
      String material = spool["filament"]["material"].as<String>();
      String name = spool["filament"]["name"].as<String>();
      if (!spool["filament"]["color_hex"].isNull()) {
        stagedSpoolColor = String("#") + spool["filament"]["color_hex"].as<String>();
      }
      stagedSpoolInfo = vendor + " " + material + " (" + name + ")";
    }
  }
  http.end();
}

bool updateSensorStates(bool currentStates[4]) {
  if (WiFi.status() != WL_CONNECTED) {
    lastMoonrakerOk = false;
    return false;
  }

  HTTPClient http;
  http.setTimeout(1500); // 1.5-second timeout
  String url = String("http://") + printerIp + ":" + String(moonrakerPort) + "/printer/objects/query?";
  for (int i = 0; i < 4; i++) {
    url += SENSOR_QUERY_NAMES[i];
    if (i < 3) url += "&";
  }

  http.begin(url);
  int httpCode = http.GET();
  String payload = "";

  if (httpCode == 200) {
    payload = http.getString();
    http.end();
    lastMoonrakerOk = true;
  } else {
    http.end();
    // Try fallback switch sensor names
    url = String("http://") + printerIp + ":" + String(moonrakerPort) + "/printer/objects/query?";
    for (int i = 0; i < 4; i++) {
      url += FALLBACK_QUERY_NAMES[i];
      if (i < 3) url += "&";
    }
    http.begin(url);
    httpCode = http.GET();
    if (httpCode == 200) {
      payload = http.getString();
      http.end();
      lastMoonrakerOk = true;
    } else {
      http.end();
      lastMoonrakerOk = false;
      return false;
    }
  }

  JsonDocument doc;
  if (deserializeJson(doc, payload)) return false;

  JsonObject status = doc["result"]["status"];
  for (int i = 0; i < 4; i++) {
    if (!status[SENSOR_KEY_NAMES[i]].isNull()) {
      currentStates[i] = status[SENSOR_KEY_NAMES[i]]["filament_detected"].as<bool>();
    } else if (!status[FALLBACK_KEY_NAMES[i]].isNull()) {
      currentStates[i] = status[FALLBACK_KEY_NAMES[i]]["filament_detected"].as<bool>();
    }
  }
  return true;
}

bool registerSpoolToPaxx(int channel, const String& uid, const String& type) {
  HTTPClient http;
  http.setTimeout(1500);
  String url = String("http://") + printerIp + ":" + String(moonrakerPort) + "/printer/filament_detect/set";
  http.begin(url);
  http.addHeader("Content-Type", "application/json");

  JsonDocument payload;
  payload["channel"] = channel;
  JsonObject info = payload["info"].to<JsonObject>();
  info["CARD_UID"] = uid;
  info["CARD_TYPE"] = type;

  String body;
  serializeJson(payload, body);

  int code = http.POST(body);
  http.end();
  return (code == 200);
}

// --- Web Server Route Handlers ---
void handleRoot() {
  server.send(200, "text/html", INDEX_HTML);
}

void handleApiStatus() {
  JsonDocument doc;
  doc["state"] = (currentState == STATE_ARMED) ? "ARMED" : "IDLE";
  doc["staged_uid"] = stagedUID;
  doc["staged_type"] = stagedType;
  doc["spool_id"] = stagedSpoolId;
  doc["spool_info"] = stagedSpoolInfo;
  doc["spool_color"] = stagedSpoolColor;
  doc["spoolman_url"] = spoolmanUrl;
  doc["is_ap_mode"] = isApMode;

  char verBuf[8];
  snprintf(verBuf, sizeof(verBuf), "0x%02X", rfidVersion);
  doc["rfid_version"] = verBuf;
  doc["rfid_connected"] = rfidConnected;
  doc["rfid_scans"] = rfidScanCount;
  doc["last_scanned_uid"] = lastScannedUid;

  if (currentState == STATE_ARMED) {
    unsigned long elapsed = millis() - armedTimestamp;
    doc["armed_remaining_s"] = (elapsed < stageTimeoutMs) ? ((stageTimeoutMs - elapsed) / 1000) : 0;
  } else {
    doc["armed_remaining_s"] = 0;
  }

  JsonArray sensors = doc["sensors"].to<JsonArray>();
  for (int i = 0; i < 4; i++) {
    JsonObject s = sensors.add<JsonObject>();
    s["channel"] = i;
    s["name"] = String("E") + i;
    s["detected"] = previousSensorState[i];
  }

  doc["moonraker_ok"] = lastMoonrakerOk;
  doc["manual_led_mode"] = manualLedMode;
  doc["led_brightness"] = currentBrightness;

  JsonArray leds = doc["leds"].to<JsonArray>();
  for (int i = 0; i < NUM_LEDS; i++) {
    leds.add(getPixelHex(i));
  }

  doc["wifi_ssid"] = isApMode ? "Snapmaker-RFID-Setup (AP)" : WiFi.SSID();
  doc["wifi_ip"] = isApMode ? WiFi.softAPIP().toString() : WiFi.localIP().toString();
  doc["wifi_rssi"] = isApMode ? 0 : WiFi.RSSI();
  doc["free_heap"] = ESP.getFreeHeap();
  doc["uptime_s"] = millis() / 1000;

  String res;
  serializeJson(doc, res);
  server.send(200, "application/json", res);
}

void handleApiConfig() {
  if (server.method() == HTTP_GET) {
    JsonDocument doc;
    doc["ssid"] = (wifiSsid == "YOUR_WIFI_SSID") ? "" : wifiSsid;
    doc["has_password"] = (wifiPassword.length() > 0 && wifiPassword != "YOUR_WIFI_PASSWORD");
    doc["printer_ip"] = printerIp;
    doc["port"] = moonrakerPort;
    doc["spoolman_url"] = spoolmanUrl;
    doc["timeout"] = stageTimeoutMs / 1000;
    doc["brightness"] = currentBrightness;
    doc["is_ap_mode"] = isApMode;

    String res;
    serializeJson(doc, res);
    server.send(200, "application/json", res);
    return;
  }

  if (server.method() == HTTP_POST) {
    String newSsid    = server.hasArg("ssid") ? server.arg("ssid") : "";
    String newPass    = server.hasArg("pass") ? server.arg("pass") : "";
    String newIp      = server.hasArg("printer_ip") ? server.arg("printer_ip") : "";
    int newPort       = server.hasArg("port") ? server.arg("port").toInt() : moonrakerPort;
    String newSpoolman = server.hasArg("spoolman_url") ? server.arg("spoolman_url") : spoolmanUrl;
    int newTimeout    = server.hasArg("timeout") ? server.arg("timeout").toInt() : 120;

    saveStoredConfig(newSsid, newPass, newIp, newPort, newSpoolman, newTimeout);

    server.send(200, "application/json", "{\"status\":\"ok\",\"rebooting\":true}");

    // Initiate scheduled reboot
    shouldReboot = true;
    rebootTimer = millis();
  }
}

void handleApiLed() {
  if (server.hasArg("mode")) {
    String mode = server.arg("mode");
    if (mode.equalsIgnoreCase("auto")) {
      manualLedMode = false;
    } else if (mode.equalsIgnoreCase("manual")) {
      manualLedMode = true;
    }
  }

  if (server.hasArg("brightness")) {
    int b = server.arg("brightness").toInt();
    if (b >= 0 && b <= 255) {
      currentBrightness = (uint8_t)b;
      pixels.setBrightness(currentBrightness);
      pixels.show();
      prefs.begin("snapmaker", false);
      prefs.putInt("brightness", currentBrightness);
      prefs.end();
    }
  }

  if (server.hasArg("action")) {
    String action = server.arg("action");
    if (action.equalsIgnoreCase("flash_green")) {
      flashFeedback(pixels.Color(0, 220, 0), 3, 120);
    } else if (action.equalsIgnoreCase("flash_red")) {
      flashFeedback(pixels.Color(220, 0, 0), 3, 120);
    } else if (action.equalsIgnoreCase("chase")) {
      manualLedMode = true;
      runChaseTest();
    } else if (action.equalsIgnoreCase("breathe")) {
      manualLedMode = true;
      runBreatheTest();
    } else if (action.equalsIgnoreCase("clear")) {
      manualLedMode = true;
      setAllPixels(0);
    } else if (action.equalsIgnoreCase("auto")) {
      manualLedMode = false;
    }
  }

  if (server.hasArg("color")) {
    manualLedMode = true;
    uint32_t col = parseHexColor(server.arg("color"));
    int idx = server.hasArg("index") ? server.arg("index").toInt() : -1;

    if (idx >= 0 && idx < NUM_LEDS) {
      pixels.setPixelColor(idx, col);
    } else {
      for (int i = 0; i < NUM_LEDS; i++) {
        pixels.setPixelColor(i, col);
      }
    }
    pixels.show();
  }

  server.send(200, "application/json", "{\"status\":\"ok\"}");
}

void handleApiRfidRaw() {
  uint8_t v = rfid.PCD_ReadRegister(MFRC522::VersionReg);
  uint8_t cmd = rfid.PCD_ReadRegister(MFRC522::CommandReg);
  uint8_t mode = rfid.PCD_ReadRegister(MFRC522::ModeReg);
  uint8_t tx = rfid.PCD_ReadRegister(MFRC522::TxControlReg);

  JsonDocument doc;
  char buf[8];
  snprintf(buf, sizeof(buf), "0x%02X", v); doc["version"] = buf;
  snprintf(buf, sizeof(buf), "0x%02X", cmd); doc["command"] = buf;
  snprintf(buf, sizeof(buf), "0x%02X", mode); doc["mode"] = buf;
  snprintf(buf, sizeof(buf), "0x%02X", tx); doc["tx_control"] = buf;
  doc["connected"] = (v != 0x00 && v != 0xFF);

  String res;
  serializeJson(doc, res);
  server.send(200, "application/json", res);
}

void handleApiAction() {
  if (server.hasArg("action")) {
    String action = server.arg("action");
    if (action.equalsIgnoreCase("arm")) {
      currentState = STATE_ARMED;
      stagedUID = server.hasArg("uid") ? server.arg("uid") : "04A1B2C3";
      stagedType = server.hasArg("type") ? server.arg("type") : "M1";
      armedTimestamp = millis();
      animStep = 0;
      manualLedMode = false;
      querySpoolmanForTag(stagedUID);
    } else if (action.equalsIgnoreCase("disarm")) {
      currentState = STATE_IDLE;
      stagedUID = "";
      stagedSpoolId = 0;
      stagedSpoolInfo = "";
      stagedSpoolColor = "";
      manualLedMode = false;
    } else if (action.equalsIgnoreCase("refresh_sensors")) {
      updateSensorStates(previousSensorState);
    } else if (action.equalsIgnoreCase("reboot")) {
      server.send(200, "application/json", "{\"status\":\"ok\",\"rebooting\":true}");
      delay(200);
      ESP.restart();
      return;
    }
  }
  server.send(200, "application/json", "{\"status\":\"ok\"}");
}

void setup() {
  Serial.begin(115200);

  // Load config from NVS Flash
  loadStoredConfig();

  // Initialize LEDs
  pixels.begin();
  pixels.setBrightness(currentBrightness);
  setAllPixels(pixels.Color(20, 20, 20)); // Soft boot indicator

  // Initialize RFID hardware
  initRfidHardware();
  Serial.printf("RC522 Version: 0x%02X (%s)\n", rfidVersion, rfidConnected ? "Connected" : "COMMUNICATION ERROR - Check Wiring!");

  // Connect to Wi-Fi with fallback to SoftAP
  WiFi.mode(WIFI_STA);
  bool wifiConfigured = (wifiSsid.length() > 0 && wifiSsid != "YOUR_WIFI_SSID");

  if (wifiConfigured) {
    Serial.printf("Connecting to Wi-Fi '%s'...\n", wifiSsid.c_str());
    WiFi.begin(wifiSsid.c_str(), wifiPassword.c_str());

    unsigned long startAttempt = millis();
    // Pulse purple on LED 6 while attempting connection (15 second timeout)
    while (WiFi.status() != WL_CONNECTED && millis() - startAttempt < 15000) {
      pixels.setPixelColor(CENTER_LED_INDEX, pixels.Color(60, 0, 80));
      pixels.show();
      delay(200);
      pixels.setPixelColor(CENTER_LED_INDEX, 0);
      pixels.show();
      delay(200);
    }
  }

  if (WiFi.status() == WL_CONNECTED) {
    isApMode = false;
    Serial.println("\nWiFi connected!");
    Serial.printf("IP address: %s\n", WiFi.localIP().toString().c_str());

    if (MDNS.begin(MDNS_NAME)) {
      Serial.printf("mDNS responder started: http://%s.local\n", MDNS_NAME);
      MDNS.addService("http", "tcp", 80);
    }
  } else {
    // Wi-Fi failed or unconfigured -> Start SoftAP setup hotspot
    isApMode = true;
    Serial.println("\nWi-Fi connection failed or not configured. Starting Fallback Hotspot...");
    WiFi.mode(WIFI_AP);
    WiFi.softAP("Snapmaker-RFID-Setup");
    IPAddress apIP = WiFi.softAPIP();
    Serial.printf("Hotspot SSID: Snapmaker-RFID-Setup\n");
    Serial.printf("Access Point IP: %s\n", apIP.toString().c_str());

    MDNS.begin(MDNS_NAME);
    MDNS.addService("http", "tcp", 80);
  }

  // Setup Web Server Routes
  server.on("/", HTTP_GET, handleRoot);
  server.on("/api/status", HTTP_GET, handleApiStatus);
  server.on("/api/config", HTTP_ANY, handleApiConfig);
  server.on("/api/rfid_raw", HTTP_GET, handleApiRfidRaw);
  server.on("/api/led", HTTP_ANY, handleApiLed);
  server.on("/api/action", HTTP_ANY, handleApiAction);
  server.begin();
  Serial.println("HTTP Web Server started on port 80");

  // Pre-seed baseline switch states if connected
  if (!isApMode) {
    updateSensorStates(previousSensorState);
  }
  pixels.clear();
  pixels.show();
}

void loop() {
  // Handle incoming web requests immediately
  server.handleClient();

  // Execute scheduled reboot if requested
  if (shouldReboot && millis() - rebootTimer > 800) {
    ESP.restart();
  }

  if (currentState == STATE_IDLE) {
    // Normal idle visuals (or cyan pulsing if in AP mode) unless in manual LED mode
    if (!manualLedMode) {
      updateIdleVisuals();
    }

    // Background sensor poll (every 4 seconds) to keep Web UI up to date
    if (!isApMode && millis() - lastSensorPoll >= 4000) {
      lastSensorPoll = millis();
      updateSensorStates(previousSensorState);
    }

    // Check RFID card presence (every 40ms)
    if (millis() - lastRfidPoll >= 40) {
      lastRfidPoll = millis();

      // Check if connection was lost and re-initialize if needed
      uint8_t currentVer = rfid.PCD_ReadRegister(MFRC522::VersionReg);
      if (currentVer == 0x00 || currentVer == 0xFF) {
        initRfidHardware();
      }

      if (isCardPresent() && rfid.PICC_ReadCardSerial()) {
        String uid = "";
        for (byte i = 0; i < rfid.uid.size; i++) {
          if (rfid.uid.uidByte[i] < 0x10) uid += "0";
          uid += String(rfid.uid.uidByte[i], HEX);
        }
        uid.toUpperCase();

        MFRC522::PICC_Type piccType = rfid.PICC_GetType(rfid.uid.sak);
        stagedType = (piccType == MFRC522::PICC_TYPE_MIFARE_UL) ? "NTAG" : "M1";
        stagedUID = uid;
        lastScannedUid = uid;
        rfidScanCount++;

        Serial.printf("RFID Tag Detected! UID: %s (%s)\n", uid.c_str(), stagedType.c_str());

        // Visual confirmation: quick blue pulse on Center LED
        pixels.setPixelColor(CENTER_LED_INDEX, pixels.Color(0, 180, 255));
        pixels.show();
        delay(70);

        rfid.PICC_HaltA();
        rfid.PCD_StopCrypto1();

        // Snapshot current sensor state at moment of tag tap
        if (!isApMode) {
          updateSensorStates(previousSensorState);
          querySpoolmanForTag(stagedUID);
        }

        currentState = STATE_ARMED;
        armedTimestamp = millis();
        animStep = 0;
        manualLedMode = false; // Resume system visuals on physical tag scan
      }
    }
  } else if (currentState == STATE_ARMED) {
    // Normal armed visual chase unless user is manually debugging LEDs
    if (!manualLedMode) {
      updateArmedVisuals();
    }

    // Staging timeout
    if (millis() - armedTimestamp > stageTimeoutMs) {
      flashFeedback(pixels.Color(180, 0, 0), 2, 200); // Red timeout flash
      currentState = STATE_IDLE;
      stagedUID = "";
      stagedSpoolId = 0;
      stagedSpoolInfo = "";
      stagedSpoolColor = "";
      return;
    }

    // Check Moonraker filament sensors frequently (every 250ms)
    if (!isApMode && millis() - lastSensorPoll >= 250) {
      lastSensorPoll = millis();

      bool currentSensorState[4];
      if (updateSensorStates(currentSensorState)) {
        for (int i = 0; i < 4; i++) {
          // Transition: false -> true (filament inserted into sensor)
          if (!previousSensorState[i] && currentSensorState[i]) {
            if (registerSpoolToPaxx(i, stagedUID, stagedType)) {
              flashFeedback(pixels.Color(0, 220, 0), 3, 150); // Triple green flash
            } else {
              flashFeedback(pixels.Color(220, 0, 0), 3, 150); // Triple red flash (API failure)
            }

            stagedUID = "";
            stagedSpoolInfo = "";
            stagedSpoolColor = "";
            currentState = STATE_IDLE;
            break;
          }
        }

        for (int i = 0; i < 4; i++) {
          previousSensorState[i] = currentSensorState[i];
        }
      }
    }
  }
}
