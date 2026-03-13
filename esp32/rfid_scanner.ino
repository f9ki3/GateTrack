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
LiquidCrystal_I2C lcd(0x27, 16, 2);

// ================= Buzzer =================
#define BUZZER_PIN 27

// ================= Relay / Light =================
#define RELAY_PIN 26
#define RELAY_ON  LOW
#define RELAY_OFF HIGH

// ================= Button Pins =================
// Wire each button from GPIO pin to GND
// Using INPUT_PULLUP = pressed means LOW
#define BTN_MODE1 12
#define BTN_MODE2 14
#define BTN_MODE3 25
#define BTN_RESET 33

#define DEBOUNCE_DELAY 180

// ================= Stepper Motor (28BYJ-48 + ULN2003) =================
// IMPORTANT: GPIO35 is INPUT ONLY on ESP32, so do not use it for stepper output.
#define STEPPER_IN1 16
#define STEPPER_IN2 17
#define STEPPER_IN3 32
#define STEPPER_IN4 13

#define STEPS_PER_REV 2048

Stepper doorStepper(STEPS_PER_REV, STEPPER_IN1, STEPPER_IN3, STEPPER_IN2, STEPPER_IN4);

// Adjust these for your actual sliding mechanism
int STEPPER_SPEED_RPM = 12;   // safe start speed for 28BYJ
int DOOR_OPEN_STEPS   = 2800;  // calibrate this for your slide distance

// ================= Mode Configuration =================
int currentMode = 1;
// 1 = Manual door toggle by button only
// 2 = Time In  (RFID -> validate -> attendance success -> light ON  -> door open 5s -> close)
// 3 = Time Out (RFID -> validate -> attendance success -> light OFF -> door open 5s -> close)

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
unsigned long lastBtn4Press = 0;

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
void buzzerOn() {
  digitalWrite(BUZZER_PIN, HIGH);
}

void buzzerOff() {
  digitalWrite(BUZZER_PIN, LOW);
}

void beepOnce(int durationMs = 120) {
  buzzerOn();
  delay(durationMs);
  buzzerOff();
}

void beepTwice() {
  beepOnce(100);
  delay(100);
  beepOnce(100);
}

void beepError() {
  beepOnce(150);
  delay(100);
  beepOnce(150);
  delay(100);
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
    Serial.println();
    Serial.println(">>> WiFi Connected!");
    Serial.print(">>> IP: ");
    Serial.println(WiFi.localIP());

    showLCD("WiFi Connected!", WiFi.localIP().toString().substring(0, 16));
    delay(2000);
  } else {
    wifiConnected = false;
    Serial.println();
    Serial.println(">>> WiFi Failed!");
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

  Serial.println(">>> WiFi reconnection failed");
  return false;
}

// ================= Door Functions =================
void openDoor(String reason) {
  if (doorIsOpen) {
    Serial.println(">>> Door already open");
    return;
  }

  Serial.println(">>> DOOR OPEN: " + reason);
  showLCD("Door Opening...", reason);
  beepOnce(100);

  stepperOpenDoor();

  doorIsOpen = true;
  showLCD("Door OPEN", reason);
}

void closeDoor(String reason) {
  if (!doorIsOpen) {
    Serial.println(">>> Door already closed");
    return;
  }

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

  if (doorIsOpen) {
    showLCD("Mode 1: Manual", "Door OPEN");
  } else {
    showLCD("Mode 1: Manual", "Door CLOSED");
  }
}

// ================= LCD State =================
void showCurrentMode() {
  if (currentMode == 1) {
    if (doorIsOpen) {
      showLCD("Mode 1: Manual", "Press Btn1 Close");
    } else {
      showLCD("Mode 1: Manual", "Press Btn1 Open");
    }
  } else if (currentMode == 2) {
    showLCD("Mode 2: Time In", "Scan RFID");
  } else if (currentMode == 3) {
    showLCD("Mode 3: Time Out", "Scan RFID");
  }
}

void setMode(int newMode) {
  currentMode = newMode;

  Serial.println(">>> MODE CHANGED TO: " + String(currentMode));

  if (currentMode == 1) {
    Serial.println(">>> Mode 1: Manual Toggle Door");
  } else if (currentMode == 2) {
    Serial.println(">>> Mode 2: Time In");
  } else if (currentMode == 3) {
    Serial.println(">>> Mode 3: Time Out");
  }

  beepOnce(80);
  delay(200);
  showCurrentMode();
}

// ================= HTTP Functions =================
bool parseResponse(String json, String& status, String& message) {
  status = "";
  message = "";

  if (json.length() < 5) return false;

  int statusIdx = json.indexOf("\"status\":\"");
  int statusOffset = 10;
  if (statusIdx == -1) {
    statusIdx = json.indexOf("\"status\": \"");
    statusOffset = 11;
  }
  if (statusIdx == -1) {
    Serial.println(">>> No status in response");
    return false;
  }

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
    if (msgEnd != -1) {
      message = json.substring(msgStart, msgEnd);
    }
  }

  Serial.println(">>> Parsed status: " + status);
  Serial.println(">>> Parsed message: " + message);
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
    Serial.println(">>> HTTP Error: " + String(httpCode) + " - " + http.errorToString(httpCode));
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

    if (!rfid.PICC_IsNewCardPresent()) {
      break;
    }
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

  // BTN 1 = Mode 1 / toggle door if already in mode 1
  if (digitalRead(BTN_MODE1) == LOW) {
    if (now - lastBtn1Press > DEBOUNCE_DELAY) {
      lastBtn1Press = now;

      Serial.println(">>> BUTTON MODE1 PRESSED");

      if (currentMode != 1) {
        setMode(1);
      } else {
        toggleDoor();
      }

      while (digitalRead(BTN_MODE1) == LOW) {
        delay(10);
      }
    }
  }

  // BTN 2 = Mode 2
  if (digitalRead(BTN_MODE2) == LOW) {
    if (now - lastBtn2Press > DEBOUNCE_DELAY) {
      lastBtn2Press = now;
      setMode(2);

      while (digitalRead(BTN_MODE2) == LOW) {
        delay(10);
      }
    }
  }

  // BTN 3 = Mode 3
  if (digitalRead(BTN_MODE3) == LOW) {
    if (now - lastBtn3Press > DEBOUNCE_DELAY) {
      lastBtn3Press = now;
      setMode(3);

      while (digitalRead(BTN_MODE3) == LOW) {
        delay(10);
      }
    }
  }

  // BTN 4 = Reset
  if (digitalRead(BTN_RESET) == LOW) {
    if (now - lastBtn4Press > DEBOUNCE_DELAY) {
      lastBtn4Press = now;
      Serial.println(">>> BUTTON RESET PRESSED");
      showLCD("Resetting...", "Please wait");
      beepTwice();
      delay(800);
      ESP.restart();
    }
  }
}

// ================= Setup =================
void setup() {
  Serial.begin(115200);
  Serial.println();
  Serial.println(">>> ==============================");
  Serial.println(">>> ESP32 RFID Scanner Starting");
  Serial.println(">>> ==============================");

  // Buzzer
  pinMode(BUZZER_PIN, OUTPUT);
  buzzerOff();

  // Relay / Light
  pinMode(RELAY_PIN, OUTPUT);
  relayLightOff();

  // Buttons
  pinMode(BTN_MODE1, INPUT_PULLUP);
  pinMode(BTN_MODE2, INPUT_PULLUP);
  pinMode(BTN_MODE3, INPUT_PULLUP);
  pinMode(BTN_RESET, INPUT_PULLUP);

  // Stepper pins
  pinMode(STEPPER_IN1, OUTPUT);
  pinMode(STEPPER_IN2, OUTPUT);
  pinMode(STEPPER_IN3, OUTPUT);
  pinMode(STEPPER_IN4, OUTPUT);
  releaseStepper();
  doorStepper.setSpeed(STEPPER_SPEED_RPM);

  // LCD I2C
  Wire.begin(21, 4);
  lcd.init();
  lcd.backlight();
  showLCD("Initializing...", "Please wait");

  // RFID SPI
  SPI.begin(SCK_PIN, MISO_PIN, MOSI_PIN, SS_PIN);
  rfid.PCD_Init();
  Serial.println(">>> RFID Reader initialized");

  // WiFi
  connectWiFi();

  showCurrentMode();

  Serial.println(">>> READY");
  Serial.println(">>> Buttons:");
  Serial.println(">>> BTN1=Mode1 / Toggle Door");
  Serial.println(">>> BTN2=Mode2");
  Serial.println(">>> BTN3=Mode3");
  Serial.println(">>> BTN4=Reset");
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

  // In MODE 1: NO RFID needed
  if (currentMode == 1) {
    delay(20);
    return;
  }

  // MODE 2 and MODE 3 need RFID
  if (!rfid.PICC_IsNewCardPresent()) {
    delay(20);
    return;
  }

  if (!rfid.PICC_ReadCardSerial()) {
    delay(20);
    return;
  }

  beepOnce(120);

  String uid = getUIDString();

  Serial.println();
  Serial.println(">>> ========== CARD DETECTED ==========");
  Serial.println(">>> UID: " + uid);
  Serial.println(">>> Mode: " + String(currentMode));

  if (uid == lastCardUID) {
    Serial.println(">>> Same card detected, remove first");
    showLCD("Wait...", "Remove card");
    delay(1200);
    return;
  }

  lastCardUID = uid;
  showLCD("Card Detected", "Processing...");

  // ================= MODE 2 =================
  if (currentMode == 2) {
    String status = "";
    String message = "";
    String response = sendAttendance(uid, 2, status, message);

    Serial.println(">>> Server Response: " + response);

    if (status == "denied") {
      handleDeniedOrError("ACCESS DENIED", message);
      return;
    }

    if (status != "success") {
      handleDeniedOrError("ERROR", message);
      return;
    }

    relayLightOn();
    Serial.println(">>> LIGHT ON after TIME IN success");

    openDoor("Time In");
    delay(5000);
    closeDoor("Auto Close");

    beepTwice();
    showLCD("TIME IN OK", "Door Closed");
    delay(1800);

    waitForCardRemoval();
    return;
  }

  // ================= MODE 3 =================
  if (currentMode == 3) {
    String status = "";
    String message = "";
    String response = sendAttendance(uid, 3, status, message);

    Serial.println(">>> Server Response: " + response);

    if (status == "denied") {
      handleDeniedOrError("ACCESS DENIED", message);
      return;
    }

    if (status != "success") {
      handleDeniedOrError("ERROR", message);
      return;
    }

    relayLightOff();
    Serial.println(">>> LIGHT OFF after TIME OUT success");

    openDoor("Time Out");
    delay(5000);
    closeDoor("Auto Close");

    beepTwice();
    showLCD("TIME OUT OK", "Light OFF");
    delay(1800);

    waitForCardRemoval();
    return;
  }
}