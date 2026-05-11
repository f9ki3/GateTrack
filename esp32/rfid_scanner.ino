
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
#define EEPROM_CONFIG_FLAG 122  // 0=unconfig, 1=configured

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

// ================= WiFi Configuration =================
const char* DEFAULT_WIFI_SSID = "Lleva";
const char* DEFAULT_WIFI_PASSWORD = "4koSiFyke123*";

// ================= Timeout Configuration =================
#define HTTP_TIMEOUT 8000

// ================= RC522 (ESP32) =================
#define SS_PIN   5
#define RST_PIN  22
#define SCK_PIN  18
#define MISO_PIN 19
#define MOSI_PIN 23

MFRC522 rfid(SS_PIN, RST_PIN);

// ================= LCD I2C =================
LiquidCrystal_I2C lcd(0x27, 16, 2); // SDA = 21, SCL = 4 (Set below in Wire.begin)

// ================= Buzzer =================
#define BUZZER_PIN 27

// ================= Relay / Light =================
#define RELAY_PIN 26
#define RELAY_ON  LOW
#define RELAY_OFF HIGH

// ================= Button Pins =================
#define BTN_MODE1 33
#define BTN_MODE2 25
#define BTN_MODE3 14
#define BTN_MODE4 15   // <--- 5TH BUTTON (Mode 4: Access Check)
#define BTN_RESET 12

#define DEBOUNCE_DELAY 180
#define LONG_PRESS_MS 3000

// ================= Stepper Motor (28BYJ-48 + ULN2003) =================
#define STEPPER_IN1 16
#define STEPPER_IN2 17
#define STEPPER_IN3 32
#define STEPPER_IN4 13

#define STEPS_PER_REV 2048

Stepper doorStepper(STEPS_PER_REV, STEPPER_IN4, STEPPER_IN2, STEPPER_IN3, STEPPER_IN1);

int STEPPER_SPEED_RPM = 18;  
int DOOR_OPEN_STEPS   = 4000;  

// ================= Mode Configuration =================
int currentMode = 1;

// ================= State Variables =================
bool doorIsOpen = false;
bool lightIsOn = false;
bool wifiConnected = false;
unsigned long lastWifiCheck = 0;
String lastCardUID = "";

unsigned long lastBtn1Press = 0;
unsigned long lastBtn2Press = 0;
unsigned long lastBtn3Press = 0;
unsigned long lastBtn4Press = 0;     
unsigned long lastBtnResetPress = 0; 

// ================= RFID Re-initialization =================
void reinitRFID() {
  Serial.println(">>> Re-initializing RFID reader...");
  rfid.PCD_StopCrypto1();
  rfid.PICC_HaltA();
  delay(50);
  rfid.PCD_Init();
  Serial.println(">>> RFID reader re-initialized");
}

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
  
  Serial.println("Loaded config - WiFi configured: " + String(wifiConfigured));
  Serial.println("SSID: " + savedSSID);
  Serial.println("Server IP: " + savedServerIP + ":" + savedServerPort);
}

void saveConfig() {
  // Clear buffers
  for (int i = 0; i < SSID_MAX_LEN; i++) EEPROM.write(EEPROM_SSID_ADDR + i, 0);
  for (int i = 0; i < PASS_MAX_LEN; i++) EEPROM.write(EEPROM_PASS_ADDR + i, 0);
  for (int i = 0; i < IP_MAX_LEN; i++) EEPROM.write(EEPROM_SERVER_IP + i, 0);
  for (int i = 0; i < PORT_MAX_LEN; i++) EEPROM.write(EEPROM_SERVER_PORT + i, 0);
  
  // Write SSID
  for (int i = 0; i < savedSSID.length() && i < SSID_MAX_LEN; i++) {
    EEPROM.write(EEPROM_SSID_ADDR + i, savedSSID.charAt(i));
  }
  EEPROM.write(EEPROM_SSID_ADDR + savedSSID.length(), 0);
  
  // Write PASS
  for (int i = 0; i < savedWifiPass.length() && i < PASS_MAX_LEN; i++) {
    EEPROM.write(EEPROM_PASS_ADDR + i, savedWifiPass.charAt(i));
  }
  EEPROM.write(EEPROM_PASS_ADDR + savedWifiPass.length(), 0);
  
  // Write IP
  for (int i = 0; i < savedServerIP.length() && i < IP_MAX_LEN; i++) {
    EEPROM.write(EEPROM_SERVER_IP + i, savedServerIP.charAt(i));
  }
  EEPROM.write(EEPROM_SERVER_IP + savedServerIP.length(), 0);
  
  // Write port
  for (int i = 0; i < savedServerPort.length() && i < PORT_MAX_LEN; i++) {
    EEPROM.write(EEPROM_SERVER_PORT + i, savedServerPort.charAt(i));
  }
  EEPROM.write(EEPROM_SERVER_PORT + savedServerPort.length(), 0);
  
  EEPROM.write(EEPROM_CONFIG_FLAG, 1);
  EEPROM.commit();
  Serial.println("Config saved to EEPROM");
}

void clearConfig() {
  EEPROM.begin(256);
  for (int i = 0; i < 128; i++) EEPROM.write(i, 0);
  EEPROM.commit();
  Serial.println("EEPROM config cleared");
}

void startConfigAP() {
  WiFi.mode(WIFI_AP);
  WiFi.softAP("GateTrack_Config");
  
  IPAddress IP = WiFi.softAPIP();
  Serial.print("Config AP started. Connect to GateTrack_Config");
  Serial.println(" and visit " + IP.toString());
  
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Connect WiFi:");
  lcd.setCursor(0,1);
  lcd.print("GateTrack_Config");
  delay(3000);
  
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Open browser:");
  lcd.setCursor(0,1);
  lcd.print("192.168.4.1");
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
  lcd.setCursor(0, 0);
  lcd.print(line1.substring(0, 16));
  lcd.setCursor(0, 1);
  lcd.print(line2.substring(0, 16));
}

// ================= Buzzer Functions =================
void buzzerOn() { digitalWrite(BUZZER_PIN, HIGH); }
void buzzerOff() { digitalWrite(BUZZER_PIN, LOW); }

void beepOnce(int durationMs = 120) {
  buzzerOn();
  delay(durationMs);
  buzzerOff();
}

void beepTwice() {
  beepOnce(100); delay(100); beepOnce(100);
}

void beepError() {
  beepOnce(150); delay(100);
  beepOnce(150); delay(100);
  beepOnce(150);
}

void beepLongReset() {
  for (int i = 0; i < 3; i++) {
    beepOnce(200);
    delay(200);
  }
}

// ================= Light Functions =================
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

// ================= Stepper Functions =================
void releaseStepper() {
  digitalWrite(STEPPER_IN1, LOW);
  digitalWrite(STEPPER_IN2, LOW);
  digitalWrite(STEPPER_IN3, LOW);
  digitalWrite(STEPPER_IN4, LOW);
}

void stepperOpenDoor() {
  Serial.println(">>> Stepper opening door...");
  doorStepper.setSpeed(STEPPER_SPEED_RPM);
  doorStepper.step(DOOR_OPEN_STEPS);
  releaseStepper();
}

void stepperCloseDoor() {
  Serial.println(">>> Stepper closing door...");
  doorStepper.setSpeed(STEPPER_SPEED_RPM);
  doorStepper.step(-DOOR_OPEN_STEPS);
  releaseStepper();
}

// ================= WiFi Functions =================
void connectWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(savedSSID.c_str(), savedWifiPass.c_str());

  showLCD("Connecting WiFi", "");
  Serial.println(">>> Connecting to WiFi: " + savedSSID);

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    lcd.setCursor(attempts % 16, 1);
    lcd.print(".");
    attempts++;
    Serial.print(".");
  }

  if (WiFi.status() == WL_CONNECTED) {
    wifiConnected = true;
    Serial.println("\n>>> WiFi Connected! IP: " + WiFi.localIP().toString());
    showLCD("WiFi Connected!", WiFi.localIP().toString().substring(0, 16));
    delay(2000);
  } else {
    wifiConnected = false;
    Serial.println("\n>>> WiFi Failed!");
    showLCD("WiFi Failed!", "");
    delay(2000);
  }
}

bool checkWiFiConnection() {
  if (WiFi.status() == WL_CONNECTED) {
    if (!wifiConnected) {
      wifiConnected = true;
      Serial.println(">>> WiFi reconnected");
      showLCD("WiFi Reconnect", WiFi.localIP().toString().substring(0, 16));
      delay(1500);
    }
    return true;
  }

  wifiConnected = false;
  Serial.println(">>> WiFi disconnected, reconnecting...");
  showLCD("WiFi Lost", "Reconnecting...");

  WiFi.disconnect();
  delay(100);
  WiFi.begin(savedSSID.c_str(), savedWifiPass.c_str());

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 15) {
    delay(500);
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    wifiConnected = true;
    Serial.println(">>> WiFi reconnected");
    showLCD("WiFi Reconnect", WiFi.localIP().toString().substring(0, 16));
    delay(1500);
    return true;
  }
  return false;
}

// ================= Config Server Handlers =================
void handleConfigRoot() {
  String html = PAGE_HEAD;
  html += R"rawliteral(
    <div class="container">
      <h1>GateTrack Setup</h1>
      <form action="/config" method="POST">
        <div class="form-group">
          <label>WiFi SSID</label>
          <input type="text" name="ssid" value="[SSID_VAL]" maxlength="32" required>
        </div>
        <div class="form-group">
          <label>WiFi Password</label>
          <input type="password" name="pass" value="[PASS_VAL]" maxlength="64">
        </div>
        <div class="form-group">
          <label>Flask Server IP</label>
          <input type="text" name="ip" value="[IP_VAL]" maxlength="15" required>
        </div>
        <div class="form-group">
          <label>Server Port</label>
          <input type="number" name="port" value="[PORT_VAL]" min="1" max="65535" required>
        </div>
        <button type="submit">Save & Reboot</button>
      </form>
    </div>
  </body>
  </html>
  )rawliteral";
  
  html.replace("[SSID_VAL]", savedSSID);
  html.replace("[PASS_VAL]", savedWifiPass);
  html.replace("[IP_VAL]", savedServerIP);
  html.replace("[PORT_VAL]", savedServerPort);
  
  configServer.send(200, "text/html", html);
}

void handleConfigSave() {
  savedSSID = configServer.arg("ssid");
  savedWifiPass = configServer.arg("pass");
  savedServerIP = configServer.arg("ip");
  savedServerPort = configServer.arg("port");
  
  if (savedSSID.length() == 0) {
    String html = PAGE_HEAD;
    html += R"rawliteral(
      <div class="container" style="text-align: center;">
        <h1 style="color: #ef4444;">Configuration Error</h1>
        <p>The WiFi SSID field cannot be left empty.</p>
        <button onclick="history.back()" style="margin-top: 20px; background-color: var(--text-secondary);">Go Back</button>
      </div>
    </body></html>
    )rawliteral";
    configServer.send(400, "text/html", html);
    return;
  }
  
  saveConfig();
  
  String html = PAGE_HEAD;
  html += R"rawliteral(
    <div class="container" style="text-align: center;">
      <h1 style="color: #10b981;">Configuration Saved!</h1>
      <p>The device is now rebooting. It will attempt to connect to your configured WiFi network shortly.</p>
      <p>You may now close this browser window.</p>
    </div>
  </body></html>
  )rawliteral";
  
  configServer.send(200, "text/html", html);
  
  Serial.println("Rebooting...");
  delay(3000);
  ESP.restart();
}

// ================= Door Functions =================
void openDoor(String reason) {
  if (doorIsOpen) return;
  Serial.println(">>> DOOR OPEN: " + reason);
  showLCD("Door Opening...", reason);
  beepOnce(100);
  stepperOpenDoor();
  doorIsOpen = true;
  showLCD("Door OPEN", reason);
}

void closeDoor(String reason) {
  if (!doorIsOpen) return;
  Serial.println(">>> DOOR CLOSE: " + reason);
  showLCD("Door Closing...", reason);
  beepOnce(100);
  stepperCloseDoor();
  doorIsOpen = false;
  showLCD("Door CLOSED", reason);
}

void toggleDoor() {
  if (doorIsOpen) {
    closeDoor("Manual Toggle");
  } else {
    openDoor("Manual Toggle");
  }
  delay(600);
  showLCD("Mode 1: Manual", doorIsOpen ? "Door OPEN" : "Door CLOSED");
}

// ================= LCD State =================
void showCurrentMode() {
  if (configMode) {
    showLCD("Config Mode", "192.168.4.1");
    return;
  }
  if (currentMode == 1) {
    showLCD("Mode 1: Manual", doorIsOpen ? "Press Btn1 Close" : "Press Btn1 Open");
  } else if (currentMode == 2) {
    showLCD("Mode 3: Time In", "Scan RFID");
  } else if (currentMode == 3) {
    showLCD("Mode 4: Time Out", "Scan RFID");
  } else if (currentMode == 4) {
    showLCD("Mode 2: Access", "Scan RFID");
  }
}

void setMode(int newMode) {
  currentMode = newMode;
  Serial.println(">>> MODE CHANGED TO: " + String(currentMode));
  beepOnce(80);
  delay(200);
  showCurrentMode();
}

// ================= HTTP Functions =================
bool parseResponse(String json, String& status, String& message) {
  status = ""; message = "";
  if (json.length() < 5) return false;

  int statusIdx = json.indexOf("\"status\":\"");
  int statusOffset = 10;
  if (statusIdx == -1) {
    statusIdx = json.indexOf("\"status\": \"");
    statusOffset = 11;
  }
  if (statusIdx == -1) return false;

  int statusStart = statusIdx + statusOffset;
  int statusEnd = json.indexOf("\"", statusStart);
  if (statusEnd == -1) return false;
  status = json.substring(statusStart, statusEnd);

  int msgIdx = json.indexOf("\"message\":\"");
  int msgOffset = 11;
  if (msgIdx == -1) {
    msgIdx = json.indexOf("\"message\": \"");
    msgOffset = 12;
  }
  if (msgIdx != -1) {
    int msgStart = msgIdx + msgOffset;
    int msgEnd = json.indexOf("\"", msgStart);
    if (msgEnd != -1) message = json.substring(msgStart, msgEnd);
  }
  return true;
}

String sendAttendance(String rfidTag, int mode, String& outStatus, String& outMessage) {
  if (!checkWiFiConnection()) {
    outStatus = "error";
    outMessage = "WiFi not connected";
    return "{\"status\":\"error\",\"message\":\"WiFi not connected\"}";
  }

  WiFiClient client;
  HTTPClient http;

// RFID-only endpoint (auto time-in / time-out)
  String url = "http://" + savedServerIP + ":" + savedServerPort + "/api/attendance";
  // IMPORTANT: send only RFID (no mode)
  String payload = "{\"rfid\":\"" + rfidTag + "\"}";

  Serial.println(">>> Sending to: " + url);
  Serial.println(">>> Payload: " + payload);

  http.begin(client, url);
  http.addHeader("Content-Type", "application/json");
  http.setTimeout(HTTP_TIMEOUT);

  int httpCode = http.POST(payload);
  String response = "";

  if (httpCode > 0) {
    response = http.getString();
    Serial.println(">>> HTTP Code: " + String(httpCode));
    Serial.println(">>> Response: " + response);

    if (!parseResponse(response, outStatus, outMessage)) {
      outStatus = "error";
      outMessage = "Invalid server response";
    }
  } else {
    outStatus = "error";
    outMessage = "HTTP " + String(httpCode);
    response = "{\"status\":\"error\",\"message\":\"HTTP error\"}";
  }

  http.end();
  return response;
}

// ================= Utilities =================
void waitForCardRemoval() {
  Serial.println(">>> Waiting for card removal...");
  showLCD("Done", "Remove card");

  unsigned long startWait = millis();
  while (millis() - startWait < 5000) {
    handleButtons();
    if (!rfid.PICC_IsNewCardPresent()) break;
    delay(100);
  }

  delay(200);
  reinitRFID();
  lastCardUID = "";
  showCurrentMode();
}

void handleDeniedOrError(const String& title, const String& msg) {
  beepError();
  showLCD(title, msg);
  delay(2200);

  lastCardUID = "";
  reinitRFID();
  showCurrentMode();
}

// ================= Button Handling =================
void handleButtons() {
  unsigned long now = millis();

  // BTN 1 = Mode 1 / toggle door
  if (digitalRead(BTN_MODE1) == LOW && (now - lastBtn1Press > DEBOUNCE_DELAY)) {
    lastBtn1Press = now;
    if (configMode) return;
    if (currentMode != 1) setMode(1);
    else toggleDoor();
    while (digitalRead(BTN_MODE1) == LOW) delay(10);
  }

  // BTN 2 = Mode 2 (Time In)
  if (digitalRead(BTN_MODE2) == LOW && (now - lastBtn2Press > DEBOUNCE_DELAY)) {
    lastBtn2Press = now;
    if (configMode) return;
    setMode(2);
    while (digitalRead(BTN_MODE2) == LOW) delay(10);
  }

  // BTN 3 = Mode 3 (Time Out)
  if (digitalRead(BTN_MODE3) == LOW && (now - lastBtn3Press > DEBOUNCE_DELAY)) {
    lastBtn3Press = now;
    if (configMode) return;
    setMode(3);
    while (digitalRead(BTN_MODE3) == LOW) delay(10);
  }

  // BTN 4 = Mode 4 (Access Check)
  if (digitalRead(BTN_MODE4) == LOW && (now - lastBtn4Press > DEBOUNCE_DELAY)) {
    lastBtn4Press = now;
    if (configMode) return;
    setMode(4);
    while (digitalRead(BTN_MODE4) == LOW) delay(10);
  }

  // BTN RESET - Short press = reboot, Long press = reset config
  if (digitalRead(BTN_RESET) == LOW) {
    if (resetPressStart == 0) {
      resetPressStart = now;
    } else if (now - resetPressStart > LONG_PRESS_MS && !resetHandled) {
      // Long press - reset config
      resetHandled = true;
      showLCD("Reset Config", "Long press OK");
      beepLongReset();
      clearConfig();
      delay(1000);
      ESP.restart();
    }
  } else {
    if (resetPressStart > 0 && now - resetPressStart < LONG_PRESS_MS && !resetHandled) {
      // Short press - reboot only
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
  
  pinMode(BUZZER_PIN, OUTPUT); buzzerOff();
  pinMode(RELAY_PIN, OUTPUT); relayLightOff();

  pinMode(BTN_MODE1, INPUT_PULLUP);
  pinMode(BTN_MODE2, INPUT_PULLUP);
  pinMode(BTN_MODE3, INPUT_PULLUP);
  pinMode(BTN_MODE4, INPUT_PULLUP);
  pinMode(BTN_RESET, INPUT_PULLUP);

  pinMode(STEPPER_IN1, OUTPUT);
  pinMode(STEPPER_IN2, OUTPUT);
  pinMode(STEPPER_IN3, OUTPUT);
  pinMode(STEPPER_IN4, OUTPUT);
  releaseStepper();
  doorStepper.setSpeed(STEPPER_SPEED_RPM);

  Wire.begin(21, 4);
  lcd.init();
  lcd.backlight();
  showLCD("Initializing...", "Please wait");

  SPI.begin(SCK_PIN, MISO_PIN, MOSI_PIN, SS_PIN);
  rfid.PCD_Init();

  // Load config and decide mode
  loadConfig();
  
  if (!wifiConfigured || savedSSID.length() == 0) {
    configMode = true;
    startConfigAP();
    
    // Setup config server
    configServer.on("/", handleConfigRoot);
    configServer.on("/config", HTTP_POST, handleConfigSave);
    configServer.begin();
    Serial.println(">>> Config server started");
    showCurrentMode();
  } else {
    configMode = false;
    connectWiFi();
    showCurrentMode();
  }
  
  Serial.println(">>> READY");
}

// ================= Main Loop =================
void loop() {
  handleButtons();

  if (configMode) {
    configServer.handleClient();
    delay(10);
    return;
  }

  unsigned long currentMillis = millis();
  if (currentMillis - lastWifiCheck > 60000) {
    lastWifiCheck = currentMillis;
    checkWiFiConnection();
  }

  // Mode 1 doesn't need RFID scans
  if (currentMode == 1) {
    delay(20);
    return;
  }

  // Listen for RFID cards
  if (!rfid.PICC_IsNewCardPresent() || !rfid.PICC_ReadCardSerial()) {
    delay(20);
    return;
  }

  beepOnce(120);
  String uid = getUIDString();

  if (uid == lastCardUID) {
    showLCD("Wait...", "Remove card");
    delay(1200);
    return;
  }

  lastCardUID = uid;
  showLCD("Card Detected", "Processing...");

  String status = "";
  String message = "";
  String response = sendAttendance(uid, currentMode, status, message);

  // If API denied or returned an error
  if (status == "denied") {
    handleDeniedOrError("ACCESS DENIED", message);
    return;
  }
  if (status != "success") {
    handleDeniedOrError("ERROR", message);
    return;
  }

  // ================= MODE 2: Time In =================
  if (currentMode == 2) {
    relayLightOn();
    openDoor("Time In");
    delay(5000);
    closeDoor("Auto Close");
    beepTwice();
    showLCD("TIME IN OK", "Door Closed");
    delay(1800);
  }
  // ================= MODE 3: Time Out =================
  else if (currentMode == 3) {
    relayLightOff();
    openDoor("Time Out");
    delay(5000);
    closeDoor("Auto Close");
    beepTwice();
    showLCD("TIME OUT OK", "Door Closed");
    delay(1800);
  }
  // ================= MODE 4: Access =================
  else if (currentMode == 4) {
    openDoor("Access OK");
    delay(5000);
    closeDoor("Auto Close");
    beepTwice();
    showLCD("ACCESS OK", "Door Closed");
    delay(1800);
  }

  waitForCardRemoval();
}