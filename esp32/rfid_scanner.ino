#include <SPI.h>
#include <MFRC522.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <HTTPClient.h>
#include <Stepper.h>

// ================= WiFi Configuration =================
const char* WIFI_SSID = "Lleva";
const char* WIFI_PASSWORD = "4koSiFyke123*";

// ================= Flask Server Configuration =================
const char* SERVER_IP = "192.168.1.68";
const int SERVER_PORT = 5000;

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
// Wire each button from GPIO pin to GND
// Using INPUT_PULLUP = pressed means LOW
#define BTN_MODE1 33
#define BTN_MODE2 25
#define BTN_MODE3 14
#define BTN_MODE4 15   // <--- 5TH BUTTON (Mode 4: Access Check)
#define BTN_RESET 12

#define DEBOUNCE_DELAY 180

// ================= Stepper Motor (28BYJ-48 + ULN2003) =================
#define STEPPER_IN1 16
#define STEPPER_IN2 17
#define STEPPER_IN3 32
#define STEPPER_IN4 13

#define STEPS_PER_REV 2048

Stepper doorStepper(STEPS_PER_REV, STEPPER_IN4, STEPPER_IN2, STEPPER_IN3, STEPPER_IN1);

// Adjust these for your actual sliding mechanism
int STEPPER_SPEED_RPM = 18;  
int DOOR_OPEN_STEPS   = 4000;  

// ================= Mode Configuration =================
int currentMode = 1;
// 1 = Manual door toggle by button only
// 2 = Time In  (RFID -> validate -> light ON  -> door open 5s -> close)
// 3 = Time Out (RFID -> validate -> light OFF -> door open 5s -> close)
// 4 = Access   (RFID -> validate -> door open 5s -> close)

// ================= State Variables =================
bool doorIsOpen = false;
bool lightIsOn = false;
bool wifiConnected = false;
unsigned long lastWifiCheck = 0;
String lastCardUID = "";

// button debounce timestamps
unsigned long lastBtn1Press = 0;
unsigned long lastBtn2Press = 0;
unsigned long lastBtn3Press = 0;
unsigned long lastBtn4Press = 0;     // For the new Mode 4 button
unsigned long lastBtnResetPress = 0; // For the Reset button

// ================= RFID Re-initialization =================
void reinitRFID() {
  Serial.println(">>> Re-initializing RFID reader...");
  rfid.PCD_StopCrypto1();
  rfid.PICC_HaltA();
  delay(50);
  rfid.PCD_Init();
  Serial.println(">>> RFID reader re-initialized");
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
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  showLCD("Connecting WiFi", "");
  Serial.println(">>> Connecting to WiFi...");

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
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

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
  if (currentMode == 1) {
    showLCD("Mode 1: Manual", doorIsOpen ? "Press Btn1 Close" : "Press Btn1 Open");
  } else if (currentMode == 2) {
    showLCD("Mode 2: Time In", "Scan RFID");
  } else if (currentMode == 3) {
    showLCD("Mode 3: Time Out", "Scan RFID");
  } else if (currentMode == 4) {
    showLCD("Mode 4: Access", "Scan RFID");
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

  String url = "http://" + String(SERVER_IP) + ":" + String(SERVER_PORT) + "/api/attendance";
  String payload = "{\"rfid\":\"" + rfidTag + "\",\"mode\":" + String(mode) + "}";

  // Mode 4 uses check_users endpoint (no mode parameter)
  if (mode == 4) {
    url = "http://" + String(SERVER_IP) + ":" + String(SERVER_PORT) + "/api/check_users";
    payload = "{\"rfid\":\"" + rfidTag + "\"}";
  }

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
    if (currentMode != 1) setMode(1);
    else toggleDoor();
    while (digitalRead(BTN_MODE1) == LOW) delay(10);
  }

  // BTN 2 = Mode 2 (Time In)
  if (digitalRead(BTN_MODE2) == LOW && (now - lastBtn2Press > DEBOUNCE_DELAY)) {
    lastBtn2Press = now;
    setMode(2);
    while (digitalRead(BTN_MODE2) == LOW) delay(10);
  }

  // BTN 3 = Mode 3 (Time Out)
  if (digitalRead(BTN_MODE3) == LOW && (now - lastBtn3Press > DEBOUNCE_DELAY)) {
    lastBtn3Press = now;
    setMode(3);
    while (digitalRead(BTN_MODE3) == LOW) delay(10);
  }

  // BTN 4 = Mode 4 (Access Check)
  if (digitalRead(BTN_MODE4) == LOW && (now - lastBtn4Press > DEBOUNCE_DELAY)) {
    lastBtn4Press = now;
    setMode(4);
    while (digitalRead(BTN_MODE4) == LOW) delay(10);
  }

  // BTN RESET
  if (digitalRead(BTN_RESET) == LOW && (now - lastBtnResetPress > DEBOUNCE_DELAY)) {
    lastBtnResetPress = now;
    showLCD("Resetting...", "Please wait");
    beepTwice();
    delay(800);
    ESP.restart();
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
  pinMode(BTN_MODE4, INPUT_PULLUP);  // Initialize the new 5th button
  pinMode(BTN_RESET, INPUT_PULLUP);

  pinMode(STEPPER_IN1, OUTPUT);
  pinMode(STEPPER_IN2, OUTPUT);
  pinMode(STEPPER_IN3, OUTPUT);
  pinMode(STEPPER_IN4, OUTPUT);
  releaseStepper();
  doorStepper.setSpeed(STEPPER_SPEED_RPM);

  // Note: This maps SDA to 21, and SCL to 4!
  Wire.begin(21, 4);
  lcd.init();
  lcd.backlight();
  showLCD("Initializing...", "Please wait");

  SPI.begin(SCK_PIN, MISO_PIN, MOSI_PIN, SS_PIN);
  rfid.PCD_Init();
  
  connectWiFi();
  showCurrentMode();
  
  Serial.println(">>> READY");
}

// ================= Main Loop =================
void loop() {
  handleButtons();

  unsigned long currentMillis = millis();
  if (currentMillis - lastWifiCheck > 60000) {
    lastWifiCheck = currentMillis;
    checkWiFiConnection();
    showCurrentMode();
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
    showLCD("TIME OUT OK", "Light OFF");
    delay(1800);
  }
  // ================= MODE 4: Access (New) =================
  else if (currentMode == 4) {
    // Mode 4 just opens the door for access, it doesn't touch the light relay
    openDoor("Access OK");
    delay(5000);
    closeDoor("Auto Close");
    beepTwice();
    showLCD("ACCESS GRANTED", "Door Closed");
    delay(1800);
  }

  waitForCardRemoval();
}