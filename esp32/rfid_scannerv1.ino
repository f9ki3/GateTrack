#include <SPI.h>
#include <MFRC522.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <WiFi.h>
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

#define SSID_MAX_LEN 32
#define PASS_MAX_LEN 64
#define IP_MAX_LEN   15
#define PORT_MAX_LEN 6

// ================= Config Variables =================
WebServer configServer(80);
bool configMode = false;

String savedSSID = "meowie WiFi";
String savedWifiPass = "wiee1976";
String savedServerIP = "192.168.18.217";
String savedServerPort = "5000";

// ================= Pins =================
#define SS_PIN          5
#define RST_PIN         22
#define BUZZER_PIN      27
#define RELAY_PIN       26
#define BTN_MODE1       33
#define BTN_RESET       12

#define RELAY_ON        LOW
#define RELAY_OFF       HIGH

// Stepper Motor
#define STEPPER_IN1     16
#define STEPPER_IN2     17
#define STEPPER_IN3     32
#define STEPPER_IN4     13
#define STEPS_PER_REV   2048

Stepper doorStepper(STEPS_PER_REV, STEPPER_IN4, STEPPER_IN2, STEPPER_IN3, STEPPER_IN1);

int STEPPER_SPEED_RPM = 18;
int DOOR_OPEN_STEPS   = 4000;

// ================= Objects =================
MFRC522 rfid(SS_PIN, RST_PIN);
LiquidCrystal_I2C lcd(0x27, 16, 2);

// State Variables
bool doorIsOpen = false;
bool wifiConnected = false;
String lastCardUID = "";
unsigned long lastWifiCheck = 0;
unsigned long resetPressStart = 0;
bool resetHandled = false;

// ================= Web Config UI =================
const String PAGE_HEAD = R"rawliteral(
<!DOCTYPE html>
<html data-bs-theme="light">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>GateTrack Config</title>
<style>
  [data-bs-theme="light"] { --bg-primary:#f5f6fa; --bg-secondary:#ffffff; --text-primary:#1c1e21; --primary:#3b5bff; }
  * { box-sizing:border-box; margin:0; padding:0; font-family:system-ui,sans-serif; }
  body { background:var(--bg-primary); color:var(--text-primary); display:flex; justify-content:center; align-items:center; min-height:100vh; }
  .container { background:#ffffff; padding:32px; border-radius:12px; width:100%; max-width:420px; box-shadow:0 4px 20px rgba(0,0,0,0.1); }
  h1 { text-align:center; margin-bottom:24px; }
  input { width:100%; padding:12px; margin-bottom:8px; border:1px solid #ddd; border-radius:8px; }
  button { width:100%; padding:14px; background:#3b5bff; color:white; border:none; border-radius:8px; font-size:1rem; cursor:pointer; }
</style>
</head>
<body>
)rawliteral";

// ================= EEPROM Functions =================
void loadConfig() {
  EEPROM.begin(256);
  bool configured = EEPROM.read(EEPROM_CONFIG_FLAG);
  Serial.println("=== Loading Config from EEPROM ===");
  Serial.print("Configured Flag: "); Serial.println(configured);

  if (configured == 1) {
    savedSSID = ""; savedWifiPass = ""; savedServerIP = ""; savedServerPort = "";
    for (int i = 0; i < SSID_MAX_LEN; i++) { char c = EEPROM.read(EEPROM_SSID_ADDR + i); if (c == 0) break; savedSSID += c; }
    for (int i = 0; i < PASS_MAX_LEN; i++) { char c = EEPROM.read(EEPROM_PASS_ADDR + i); if (c == 0) break; savedWifiPass += c; }
    for (int i = 0; i < IP_MAX_LEN; i++) { char c = EEPROM.read(EEPROM_SERVER_IP + i); if (c == 0) break; savedServerIP += c; }
    for (int i = 0; i < PORT_MAX_LEN; i++) { char c = EEPROM.read(EEPROM_SERVER_PORT + i); if (c == 0) break; savedServerPort += c; }
  } else {
    Serial.println("Using default values");
  }

  Serial.print("SSID: "); Serial.println(savedSSID);
  Serial.print("Server IP: "); Serial.println(savedServerIP);
}

void saveConfig() {
  for (int i = 0; i < 256; i++) EEPROM.write(i, 0);
  for (int i = 0; i < savedSSID.length() && i < SSID_MAX_LEN; i++) EEPROM.write(EEPROM_SSID_ADDR + i, savedSSID[i]);
  for (int i = 0; i < savedWifiPass.length() && i < PASS_MAX_LEN; i++) EEPROM.write(EEPROM_PASS_ADDR + i, savedWifiPass[i]);
  for (int i = 0; i < savedServerIP.length() && i < IP_MAX_LEN; i++) EEPROM.write(EEPROM_SERVER_IP + i, savedServerIP[i]);
  for (int i = 0; i < savedServerPort.length() && i < PORT_MAX_LEN; i++) EEPROM.write(EEPROM_SERVER_PORT + i, savedServerPort[i]);
  EEPROM.write(EEPROM_CONFIG_FLAG, 1);
  EEPROM.commit();
  Serial.println("Config Saved!");
}

void clearConfig() {
  EEPROM.begin(256);
  for (int i = 0; i < 256; i++) EEPROM.write(i, 0);
  EEPROM.commit();
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

void showLCD(String line1, String line2 = "") {
  lcd.clear();
  lcd.setCursor(0, 0); lcd.print(line1.substring(0,16));
  lcd.setCursor(0, 1); lcd.print(line2.substring(0,16));
}

void beepOnce(int ms = 120) { digitalWrite(BUZZER_PIN, HIGH); delay(ms); digitalWrite(BUZZER_PIN, LOW); }
void beepTwice() { beepOnce(100); delay(100); beepOnce(100); }
void beepError() { for(int i=0; i<3; i++){ beepOnce(150); delay(100); } }

void relayLightOn()  { digitalWrite(RELAY_PIN, RELAY_ON); }
void relayLightOff() { digitalWrite(RELAY_PIN, RELAY_OFF); }

void releaseStepper() {
  digitalWrite(STEPPER_IN1, LOW); digitalWrite(STEPPER_IN2, LOW);
  digitalWrite(STEPPER_IN3, LOW); digitalWrite(STEPPER_IN4, LOW);
}

void openDoor(String reason) {
  if (doorIsOpen) return;
  Serial.print(">>> OPENING DOOR: "); Serial.println(reason);
  showLCD("Opening Door", reason);
  doorStepper.setSpeed(STEPPER_SPEED_RPM);
  doorStepper.step(DOOR_OPEN_STEPS);
  releaseStepper();
  doorIsOpen = true;
  showLCD("Door OPEN", reason);
}

void closeDoor(String reason) {
  if (!doorIsOpen) return;
  Serial.print(">>> CLOSING DOOR: "); Serial.println(reason);
  showLCD("Closing Door", reason);
  doorStepper.setSpeed(STEPPER_SPEED_RPM);
  doorStepper.step(-DOOR_OPEN_STEPS);
  releaseStepper();
  doorIsOpen = false;
  showLCD("Door CLOSED", reason);
}

void toggleDoor() {
  Serial.println(">>> Manual Door Toggle");
  if (doorIsOpen) closeDoor("Manual");
  else openDoor("Manual");
}

// ================= WiFi =================
void connectWiFi() {
  Serial.println("Connecting to WiFi: " + savedSSID);
  WiFi.mode(WIFI_STA);
  WiFi.begin(savedSSID.c_str(), savedWifiPass.c_str());
  showLCD("Connecting WiFi", "");

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 25) {
    delay(500); attempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    wifiConnected = true;
    Serial.println("WiFi Connected! IP: " + WiFi.localIP().toString());
    showLCD("WiFi Connected", WiFi.localIP().toString());
  } else {
    Serial.println("WiFi Failed!");
    showLCD("WiFi Failed", "");
  }
}

bool checkWiFiConnection() {
  if (WiFi.status() == WL_CONNECTED) return true;
  WiFi.disconnect();
  delay(100);
  WiFi.begin(savedSSID.c_str(), savedWifiPass.c_str());
  delay(500);
  return (WiFi.status() == WL_CONNECTED);
}

// ================= sendRFID =================
String sendRFID(String rfidTag, String &status, String &message, String &type) {
  Serial.println("\n========== HTTP REQUEST START ==========");
  Serial.print("RFID: "); Serial.println(rfidTag);

  HTTPClient http;
  String url = "http://" + savedServerIP + ":" + savedServerPort + "/api/attendance";
  String payload = "{\"rfid\":\"" + rfidTag + "\"}";

  http.begin(url);
  http.addHeader("Content-Type", "application/json");
  http.setTimeout(10000);

  int httpCode = http.POST(payload);
  String response = http.getString();
  http.end();

  Serial.print("HTTP Code: "); Serial.println(httpCode);
  if (httpCode == 200) Serial.print("Response: "); Serial.println(response);

  status = message = type = "";

  int sIdx = response.indexOf("\"status\"");
  if (sIdx > 0) {
    int colon = response.indexOf(":", sIdx);
    int q1 = response.indexOf("\"", colon + 1);
    int q2 = response.indexOf("\"", q1 + 1);
    if (q1 > 0 && q2 > 0) status = response.substring(q1 + 1, q2);
  }

  int tIdx = response.indexOf("\"type\"");
  if (tIdx > 0) {
    int colon = response.indexOf(":", tIdx);
    int q1 = response.indexOf("\"", colon + 1);
    int q2 = response.indexOf("\"", q1 + 1);
    if (q1 > 0 && q2 > 0) type = response.substring(q1 + 1, q2);
  }

  Serial.print("Parsed Status: ["); Serial.print(status); Serial.println("]");
  Serial.print("Parsed Type  : ["); Serial.print(type); Serial.println("]");

  return response;
}

// ================= Config Server =================
void handleConfigRoot() {
  String html = PAGE_HEAD + R"rawliteral(
    <div class="container">
      <h1>GateTrack Setup</h1>
      <form action="/config" method="POST">
        <div class="form-group"><label>WiFi SSID</label><input type="text" name="ssid" value="[SSID_VAL]" required></div>
        <div class="form-group"><label>WiFi Password</label><input type="password" name="pass" value="[PASS_VAL]"></div>
        <div class="form-group"><label>Server IP</label><input type="text" name="ip" value="[IP_VAL]" required></div>
        <div class="form-group"><label>Server Port</label><input type="number" name="port" value="[PORT_VAL]" required></div>
        <button type="submit">Save & Reboot</button>
      </form>
    </div></body></html>
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
    configServer.send(400, "text/html", "<h1>SSID cannot be empty!</h1>");
    return;
  }
  saveConfig();
  configServer.send(200, "text/html", "<h1>Config Saved!<br>Rebooting...</h1>");
  delay(2000);
  ESP.restart();
}

// ================= SETUP =================
void setup() {
  Serial.begin(115200);
  Serial.println("\n\n=== GateTrack System Starting ===");

  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(RELAY_PIN, OUTPUT);
  pinMode(BTN_MODE1, INPUT_PULLUP);
  pinMode(BTN_RESET, INPUT_PULLUP);
  relayLightOff();

  pinMode(STEPPER_IN1, OUTPUT); pinMode(STEPPER_IN2, OUTPUT);
  pinMode(STEPPER_IN3, OUTPUT); pinMode(STEPPER_IN4, OUTPUT);
  releaseStepper();
  doorStepper.setSpeed(STEPPER_SPEED_RPM);

  Wire.begin(21, 4);
  lcd.init();
  lcd.backlight();

  SPI.begin(18, 19, 23, SS_PIN);
  rfid.PCD_Init();

  loadConfig();
  showLCD("GateTrack", "Initializing...");

  if (savedSSID.length() == 0 || EEPROM.read(EEPROM_CONFIG_FLAG) == 0) {
    configMode = true;
    WiFi.softAP("GateTrack_Config");
    IPAddress ip = WiFi.softAPIP();
    showLCD("Config Mode", ip.toString());
    configServer.on("/", handleConfigRoot);
    configServer.on("/config", HTTP_POST, handleConfigSave);
    configServer.begin();
  } else {
    connectWiFi();
  }

  showLCD("Ready", "Scan RFID");
  Serial.println("=== System Ready ===");
}

// ================= LOOP =================
void loop() {
  if (configMode) { configServer.handleClient(); delay(10); return; }

  if (millis() - lastWifiCheck > 60000) {
    lastWifiCheck = millis();
    checkWiFiConnection();
  }

  // Reset Button
  if (digitalRead(BTN_RESET) == LOW) {
    if (resetPressStart == 0) resetPressStart = millis();
    if (millis() - resetPressStart > 3000 && !resetHandled) {
      resetHandled = true;
      clearConfig();
      delay(1000);
      ESP.restart();
    }
  } else if (resetPressStart > 0 && millis() - resetPressStart < 800) {
    ESP.restart();
  }
  resetPressStart = 0;
  resetHandled = false;

  // Button 1 - Manual Toggle
  static unsigned long lastBtn1 = 0;
  if (digitalRead(BTN_MODE1) == LOW && millis() - lastBtn1 > 250) {
    lastBtn1 = millis();
    toggleDoor();
  }

  // RFID Scan
  if (!rfid.PICC_IsNewCardPresent() || !rfid.PICC_ReadCardSerial()) {
    delay(50); return;
  }

  beepOnce(100);
  String uid = getUIDString();
  Serial.print("RFID Detected: "); Serial.println(uid);

  if (uid == lastCardUID) { delay(800); return; }

  lastCardUID = uid;
  showLCD("Card Read", uid.substring(0,8));

  String status = "", message = "", type = "";
  sendRFID(uid, status, message, type);

  if (status == "success") {
    Serial.println("✅ SUCCESS - Type: " + type);
    beepTwice();

    if (type == "time_in") {
      relayLightOn();
      openDoor("Time In");
      delay(5000);
      closeDoor("Auto Close");
    } 
    else if (type == "time_out") {
      relayLightOff();
      openDoor("Time Out");
      delay(5000);
      closeDoor("Auto Close");
    } 
    else if (type == "already_logged") {
      // === NO ACTION FOR ALREADY LOGGED ===
      showLCD("Already Logged", "Today");
      Serial.println("Already logged today - No action taken");
      delay(3000);
    } else if (type == "someone_inside") {
      showLCD("Occupied!");
      Serial.println("Someone Inside Please try later!");
      delay(3000);
    }
    else {
      // Default access
      relayLightOn();
      openDoor("Access Granted");
      delay(5000);
      closeDoor("Auto Close");
    }
  } 
  else {
    Serial.println("❌ Access Denied");
    beepError();
    showLCD("Access Denied", "");
    delay(2200);
  }

  // Wait for card removal
  unsigned long start = millis();
  while (millis() - start < 5000 && rfid.PICC_IsNewCardPresent()) delay(100);

  rfid.PICC_HaltA();
  lastCardUID = "";
  showLCD("Ready", "Scan RFID");
}