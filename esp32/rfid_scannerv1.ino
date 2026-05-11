#include <SPI.h>
#include <MFRC522.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <HTTPClient.h>
#include <WebServer.h>
#include <EEPROM.h>
#include <Stepper.h>

// ================= EEPROM Addresses =================
#define EEPROM_SSID_ADDR     0
#define EEPROM_PASS_ADDR    32
#define EEPROM_SERVER_IP   100
#define EEPROM_SERVER_PORT 116
#define EEPROM_CONFIG_FLAG 122

// EEPROM sizes
#define SSID_MAX_LEN 32
#define PASS_MAX_LEN 64
#define IP_MAX_LEN   15
#define PORT_MAX_LEN 6

// ================= Config Variables =================
WebServer configServer(80);
bool configMode = false;
bool wifiConfigured = false;
String savedSSID = "";
String savedWifiPass = "";
String savedServerIP = "192.168.1.68";
String savedServerPort = "5000";

unsigned long resetPressStart = 0;
bool resetHandled = false;

// ================= Web Interface UI Theme =================
const String PAGE_HEAD = R"rawliteral(
<!DOCTYPE html>
<html data-bs-theme="light">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>GateTrack Config</title>
<style>
  [data-bs-theme="light"] {
    --bg-primary: #f5f6fa;
    --bg-secondary: #ffffff;
    --text-primary: #1c1e21;
    --text-secondary: #6c757d;
    --primary: #3b5bff;
    --primary-hover: #2f49d1;
    --border-color: #e5e7eb;
    --card-shadow: 0 1px 2px rgba(0, 0, 0, 0.04), 0 6px 16px rgba(0, 0, 0, 0.06);
  }
  [data-bs-theme="dark"] {
    --bg-primary: #121317;
    --bg-secondary: #1f2126;
    --text-primary: #f1f3f5;
    --text-secondary: #9aa0a6;
    --primary: #4c6fff;
    --primary-hover: #5c7cff;
    --border-color: #2b2f36;
    --card-shadow: 0 4px 14px rgba(0, 0, 0, 0.45);
  }
  * { box-sizing: border-box; margin: 0; padding: 0; font-family: system-ui, -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, sans-serif; }
  body { background-color: var(--bg-primary); color: var(--text-primary); display: flex; justify-content: center; align-items: center; min-height: 100vh; padding: 20px; }
  .container { background-color: var(--bg-secondary); box-shadow: var(--card-shadow); border-radius: 12px; padding: 32px; width: 100%; max-width: 420px; }
  h1 { font-size: 1.5rem; margin-bottom: 24px; text-align: center; color: var(--text-primary); }
  .form-group { margin-bottom: 16px; text-align: left; }
  label { display: block; margin-bottom: 6px; color: var(--text-secondary); font-size: 0.9rem; font-weight: 500; }
  input { width: 100%; padding: 12px; border: 1px solid var(--border-color); border-radius: 8px; background-color: var(--bg-primary); color: var(--text-primary); font-size: 1rem; outline: none; transition: border-color 0.2s, box-shadow 0.2s; }
  input:focus { border-color: var(--primary); box-shadow: 0 0 0 3px rgba(59, 91, 255, 0.1); }
  button { width: 100%; padding: 14px; background-color: var(--primary); color: #fff; border: none; border-radius: 8px; font-size: 1rem; font-weight: 600; cursor: pointer; transition: background-color 0.2s; margin-top: 10px; }
  button:hover { background-color: var(--primary-hover); }
  p, .note { font-size: 0.95rem; color: var(--text-secondary); margin-top: 15px; line-height: 1.5; text-align: center; }
</style>
<script>
  if (window.matchMedia && window.matchMedia('(prefers-color-scheme: dark)').matches) {
    document.documentElement.setAttribute('data-bs-theme', 'dark');
  }
</script>
</head>
<body>
)rawliteral";

// ================= Pins =================
#define BUZZER_PIN 27
#define RELAY_PIN  26
#define RELAY_ON   LOW
#define RELAY_OFF  HIGH

#define BTN_MODE1 33   // Button 1: Manual Door Toggle
#define BTN_RESET 12

#define SS_PIN   5
#define RST_PIN  22
#define SCK_PIN  18
#define MISO_PIN 19
#define MOSI_PIN 23

// Stepper
#define STEPPER_IN1 16
#define STEPPER_IN2 17
#define STEPPER_IN3 32
#define STEPPER_IN4 13
#define STEPS_PER_REV 2048

// ================= Objects =================
MFRC522 rfid(SS_PIN, RST_PIN);
LiquidCrystal_I2C lcd(0x27, 16, 2);
Stepper doorStepper(STEPS_PER_REV, STEPPER_IN4, STEPPER_IN2, STEPPER_IN3, STEPPER_IN1);

int STEPPER_SPEED_RPM = 18;
int DOOR_OPEN_STEPS   = 4000;

// ================= State Variables =================
bool doorIsOpen = false;
bool lightIsOn = false;
bool wifiConnected = false;
unsigned long lastWifiCheck = 0;
String lastCardUID = "";

unsigned long lastBtn1Press = 0;
unsigned long resetPressStart = 0;
bool resetHandled = false;

#define DEBOUNCE_DELAY 180
#define LONG_PRESS_MS  3000
#define HTTP_TIMEOUT   8000

// ================= EEPROM Functions =================
void loadConfig() {
  EEPROM.begin(256);
  wifiConfigured = EEPROM.read(EEPROM_CONFIG_FLAG);
  
  savedSSID = "";
  for (int i = 0; i < SSID_MAX_LEN; i++) {
    char c = EEPROM.read(EEPROM_SSID_ADDR + i);
    if (c == 0) break;
    savedSSID += c;
  }
  
  savedWifiPass = "";
  for (int i = 0; i < PASS_MAX_LEN; i++) {
    char c = EEPROM.read(EEPROM_PASS_ADDR + i);
    if (c == 0) break;
    savedWifiPass += c;
  }
  
  savedServerIP = "";
  for (int i = 0; i < IP_MAX_LEN; i++) {
    char c = EEPROM.read(EEPROM_SERVER_IP + i);
    if (c == 0) break;
    savedServerIP += c;
  }
  
  savedServerPort = "";
  for (int i = 0; i < PORT_MAX_LEN; i++) {
    char c = EEPROM.read(EEPROM_SERVER_PORT + i);
    if (c == 0) break;
    savedServerPort += c;
  }
  
  if (savedServerIP == "") savedServerIP = "192.168.1.68";
  if (savedServerPort == "") savedServerPort = "5000";
}

void saveConfig() {
  for (int i = 0; i < SSID_MAX_LEN; i++) EEPROM.write(EEPROM_SSID_ADDR + i, 0);
  for (int i = 0; i < PASS_MAX_LEN; i++) EEPROM.write(EEPROM_PASS_ADDR + i, 0);
  for (int i = 0; i < IP_MAX_LEN; i++) EEPROM.write(EEPROM_SERVER_IP + i, 0);
  for (int i = 0; i < PORT_MAX_LEN; i++) EEPROM.write(EEPROM_SERVER_PORT + i, 0);
  
  for (int i = 0; i < savedSSID.length() && i < SSID_MAX_LEN; i++) EEPROM.write(EEPROM_SSID_ADDR + i, savedSSID.charAt(i));
  EEPROM.write(EEPROM_SSID_ADDR + savedSSID.length(), 0);
  
  for (int i = 0; i < savedWifiPass.length() && i < PASS_MAX_LEN; i++) EEPROM.write(EEPROM_PASS_ADDR + i, savedWifiPass.charAt(i));
  EEPROM.write(EEPROM_PASS_ADDR + savedWifiPass.length(), 0);
  
  for (int i = 0; i < savedServerIP.length() && i < IP_MAX_LEN; i++) EEPROM.write(EEPROM_SERVER_IP + i, savedServerIP.charAt(i));
  EEPROM.write(EEPROM_SERVER_IP + savedServerIP.length(), 0);
  
  for (int i = 0; i < savedServerPort.length() && i < PORT_MAX_LEN; i++) EEPROM.write(EEPROM_SERVER_PORT + i, savedServerPort.charAt(i));
  EEPROM.write(EEPROM_SERVER_PORT + savedServerPort.length(), 0);
  
  EEPROM.write(EEPROM_CONFIG_FLAG, 1);
  EEPROM.commit();
}

void clearConfig() {
  EEPROM.begin(256);
  for (int i = 0; i < 128; i++) EEPROM.write(i, 0);
  EEPROM.commit();
}

void startConfigAP() {
  WiFi.mode(WIFI_AP);
  WiFi.softAP("GateTrack_Config");
  IPAddress IP = WiFi.softAPIP();
  
  lcd.clear();
  lcd.setCursor(0,0); lcd.print("Connect WiFi:");
  lcd.setCursor(0,1); lcd.print("GateTrack_Config");
  delay(3000);
  
  lcd.clear();
  lcd.setCursor(0,0); lcd.print("Open browser:");
  lcd.setCursor(0,1); lcd.print("192.168.4.1");
}

// ================= Helpers =================
String getUIDString() {
  String uid = "";
  for (byte i = 0; i < rfid.uid.size; i++) {
    if (rfid.uid.uidByte[i] < 0x10) uid += "0";
    uid += String(rfid.uid.uidByte[i], HEX);
  }
  uid.toUpperCase();
  return uid;
}

void showLCD(const String& line1, const String& line2 = "") {
  lcd.clear();
  lcd.setCursor(0, 0); lcd.print(line1.substring(0, 16));
  lcd.setCursor(0, 1); lcd.print(line2.substring(0, 16));
}

// ================= Buzzer =================
void beepOnce(int durationMs = 120) {
  digitalWrite(BUZZER_PIN, HIGH);
  delay(durationMs);
  digitalWrite(BUZZER_PIN, LOW);
}

void beepTwice() {
  beepOnce(100); delay(100); beepOnce(100);
}

void beepError() {
  beepOnce(150); delay(100); beepOnce(150); delay(100); beepOnce(150);
}

void beepLongReset() {
  for (int i = 0; i < 3; i++) { beepOnce(200); delay(200); }
}

// ================= Light & Door =================
void relayLightOn() {
  digitalWrite(RELAY_PIN, RELAY_ON);
  lightIsOn = true;
  Serial.println(">>> LIGHT ON");
}

void relayLightOff() {
  digitalWrite(RELAY_PIN, RELAY_OFF);
  lightIsOn = false;
  Serial.println(">>> LIGHT OFF");
}

void releaseStepper() {
  digitalWrite(STEPPER_IN1, LOW); digitalWrite(STEPPER_IN2, LOW);
  digitalWrite(STEPPER_IN3, LOW); digitalWrite(STEPPER_IN4, LOW);
}

void stepperOpenDoor() {
  Serial.println(">>> Opening door...");
  doorStepper.setSpeed(STEPPER_SPEED_RPM);
  doorStepper.step(DOOR_OPEN_STEPS);
  releaseStepper();
}

void stepperCloseDoor() {
  Serial.println(">>> Closing door...");
  doorStepper.setSpeed(STEPPER_SPEED_RPM);
  doorStepper.step(-DOOR_OPEN_STEPS);
  releaseStepper();
}

void openDoor(String reason) {
  if (doorIsOpen) return;
  showLCD("Door Opening...", reason);
  beepOnce(100);
  stepperOpenDoor();
  doorIsOpen = true;
  showLCD("Door OPEN", reason);
}

void closeDoor(String reason) {
  if (!doorIsOpen) return;
  showLCD("Door Closing...", reason);
  beepOnce(100);
  stepperCloseDoor();
  doorIsOpen = false;
  showLCD("Door CLOSED", reason);
}

void toggleDoor() {
  if (doorIsOpen) closeDoor("Manual");
  else openDoor("Manual");
  delay(600);
  showLCD("Manual Mode", doorIsOpen ? "Door OPEN" : "Door CLOSED");
}

// ================= HTTP =================
bool parseResponse(String json, String& status, String& message, String& type) {
  status = message = type = "";
  int sIdx = json.indexOf("\"status\":\"");
  if (sIdx != -1) {
    int start = sIdx + 10;
    int end = json.indexOf("\"", start);
    if (end != -1) status = json.substring(start, end);
  }

  int tIdx = json.indexOf("\"type\":\"");
  if (tIdx != -1) {
    int start = tIdx + 8;
    int end = json.indexOf("\"", start);
    if (end != -1) type = json.substring(start, end);
  }

  int mIdx = json.indexOf("\"message\":\"");
  if (mIdx != -1) {
    int start = mIdx + 11;
    int end = json.indexOf("\"", start);
    if (end != -1) message = json.substring(start, end);
  }
  return status.length() > 0;
}

String sendRFID(String rfidTag, String& outStatus, String& outMessage, String& outType) {
  if (!wifiConnected && !checkWiFiConnection()) {
    outStatus = "error";
    outMessage = "WiFi not connected";
    return "{\"status\":\"error\"}";
  }

  WiFiClient client;
  HTTPClient http;
  String url = "http://" + savedServerIP + ":" + savedServerPort + "/api/attendance";
  String payload = "{\"rfid\":\"" + rfidTag + "\"}";

  http.begin(client, url);
  http.addHeader("Content-Type", "application/json");
  http.setTimeout(HTTP_TIMEOUT);

  int httpCode = http.POST(payload);
  String response = http.getString();
  http.end();

  Serial.println(">>> Response: " + response);

  if (httpCode > 0) {
    parseResponse(response, outStatus, outMessage, outType);
  } else {
    outStatus = "error";
    outMessage = "HTTP Error";
  }
  return response;
}

bool checkWiFiConnection() {
  if (WiFi.status() == WL_CONNECTED) {
    wifiConnected = true;
    return true;
  }
  // Try reconnect
  WiFi.begin(savedSSID.c_str(), savedWifiPass.c_str());
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 12) {
    delay(500);
    attempts++;
  }
  wifiConnected = (WiFi.status() == WL_CONNECTED);
  return wifiConnected;
}

// ================= Button Handling =================
void handleButtons() {
  unsigned long now = millis();

  // Button 1 - Manual Toggle
  if (digitalRead(BTN_MODE1) == LOW && (now - lastBtn1Press > DEBOUNCE_DELAY)) {
    lastBtn1Press = now;
    if (!configMode) toggleDoor();
    while (digitalRead(BTN_MODE1) == LOW) delay(10);
  }

  // Reset Button
  if (digitalRead(BTN_RESET) == LOW) {
    if (resetPressStart == 0) resetPressStart = now;
    else if (now - resetPressStart > LONG_PRESS_MS && !resetHandled) {
      resetHandled = true;
      showLCD("Reset Config", "Clearing...");
      beepLongReset();
      clearConfig();
      delay(1000);
      ESP.restart();
    }
  } else {
    if (resetPressStart > 0 && now - resetPressStart < LONG_PRESS_MS && !resetHandled) {
      showLCD("Rebooting...", "");
      beepTwice();
      delay(800);
      ESP.restart();
    }
    resetPressStart = 0;
    resetHandled = false;
  }
}

// ================= Setup =================
void setup() {
  Serial.begin(115200);
  
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(RELAY_PIN, OUTPUT);
  relayLightOff();

  pinMode(BTN_MODE1, INPUT_PULLUP);
  pinMode(BTN_RESET, INPUT_PULLUP);

  // Stepper
  pinMode(STEPPER_IN1, OUTPUT); pinMode(STEPPER_IN2, OUTPUT);
  pinMode(STEPPER_IN3, OUTPUT); pinMode(STEPPER_IN4, OUTPUT);
  releaseStepper();
  doorStepper.setSpeed(STEPPER_SPEED_RPM);

  Wire.begin(21, 4);
  lcd.init();
  lcd.backlight();
  showLCD("Initializing...", "");

  SPI.begin(SCK_PIN, MISO_PIN, MOSI_PIN, SS_PIN);
  rfid.PCD_Init();

  loadConfig();

  if (!wifiConfigured || savedSSID.length() == 0) {
    configMode = true;
    startConfigAP();
    configServer.on("/", []() { /* same as before */ });
    configServer.on("/config", HTTP_POST, []() { /* same as before */ });
    configServer.begin();
  } else {
    WiFi.mode(WIFI_STA);
    WiFi.begin(savedSSID.c_str(), savedWifiPass.c_str());
    checkWiFiConnection();
  }

  showLCD("Ready", "Btn1=Manual | RFID=Auto");
  Serial.println(">>> GateTrack Ready - RFID Auto + Manual Button");
}

// ================= Main Loop =================
void loop() {
  handleButtons();

  if (configMode) {
    configServer.handleClient();
    delay(10);
    return;
  }

  // WiFi check
  if (millis() - lastWifiCheck > 60000) {
    lastWifiCheck = millis();
    checkWiFiConnection();
  }

  // RFID Scan
  if (!rfid.PICC_IsNewCardPresent() || !rfid.PICC_ReadCardSerial()) {
    delay(20);
    return;
  }

  beepOnce(120);
  String uid = getUIDString();

  if (uid == lastCardUID) {
    delay(800);
    return;
  }

  lastCardUID = uid;
  showLCD("Card Detected", "Processing...");

  String status = "", message = "", type = "";
  sendRFID(uid, status, message, type);

  if (status == "denied" || status == "error") {
    beepError();
    showLCD("ACCESS DENIED", message.substring(0,16));
    delay(2200);
    lastCardUID = "";
    rfid.PCD_Init();
    showLCD("Ready", "Btn1=Manual | RFID=Auto");
    return;
  }

  // ================= AUTOMATIC BEHAVIOR =================
  if (type == "time_in") {
    relayLightOn();
    openDoor("Time In");
    delay(5000);
    closeDoor("Auto Close");
    beepTwice();
    showLCD("TIME IN OK", "");
  }
  else if (type == "time_out") {
    openDoor("Time Out");
    delay(5000);
    closeDoor("Auto Close");
    relayLightOff();
    beepTwice();
    showLCD("TIME OUT OK", "");
  }
  else {
    // Fallback (e.g. already logged, access only, etc.)
    openDoor("Access");
    delay(5000);
    closeDoor("Auto Close");
    beepTwice();
    showLCD("ACCESS OK", "");
  }

  // Wait for card removal
  unsigned long startWait = millis();
  while (millis() - startWait < 5000) {
    handleButtons();
    if (!rfid.PICC_IsNewCardPresent()) break;
    delay(100);
  }

  lastCardUID = "";
  rfid.PCD_Init();
  showLCD("Ready", "Btn1=Manual | RFID=Auto");
}