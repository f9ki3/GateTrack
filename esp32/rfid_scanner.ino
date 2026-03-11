#include <SPI.h>
#include <MFRC522.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <HTTPClient.h>

// ================= WiFi Configuration =================
const char* WIFI_SSID = "Lleva";
const char* WIFI_PASSWORD = "4koSiFyke123*";

// ================= Flask Server Configuration =================
const char* SERVER_IP = "192.168.1.68";
const int SERVER_PORT = 5000;

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

// ================= Relay =================
#define RELAY_PIN 26

// Most 5V relay modules are ACTIVE LOW
#define RELAY_ON  LOW
#define RELAY_OFF HIGH

// ================= Mode Configuration =================
int currentMode = 2;
// 1 = Door Only
// 2 = Time In  -> Light ON
// 3 = Time Out -> Light OFF

// ================= State Variables =================
bool doorSimulatedOpen = false;
bool lightSimulatedOn = false;

// ================= Helpers =================
String getUIDString() {
  String uid = "";
  for (byte i = 0; i < rfid.uid.size; i++) {
    if (rfid.uid.uidByte[i] < 0x10) uid += "0";
    uid += String(rfid.uid.uidByte[i], HEX);
    if (i < rfid.uid.size - 1) uid += " ";
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

void beepTwiceShort() {
  beepOnce(100);
  delay(100);
  beepOnce(100);
}

void beepLong() {
  beepOnce(500);
}

// ================= Relay / Light Functions =================
void relayLightOn() {
  digitalWrite(RELAY_PIN, RELAY_ON);
  lightSimulatedOn = true;
  Serial.println(">>> RELAY LIGHT ON");
}

void relayLightOff() {
  digitalWrite(RELAY_PIN, RELAY_OFF);
  lightSimulatedOn = false;
  Serial.println(">>> RELAY LIGHT OFF");
}

void connectWiFi() {
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Connecting WiFi");

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    lcd.setCursor(attempts % 16, 1);
    lcd.print(".");
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("WiFi Connected!");
    lcd.setCursor(0, 1);
    lcd.print(WiFi.localIP().toString().substring(0, 16));
    delay(2000);
  } else {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("WiFi Failed!");
    delay(2000);
  }
}

void simulateEmergencyDoor();

void handleSerialCommands() {
  if (Serial.available() > 0) {
    String command = Serial.readStringUntil('\n');
    command.trim();

    if (command == "4") {
      Serial.println(">>> EMERGENCY: Toggling door NOW");
      simulateEmergencyDoor();
      return;
    }

    if (command == "1" || command == "2" || command == "3") {
      currentMode = command.toInt();

      if (currentMode == 1) {
        Serial.println(">>> MODE: 1 - Door Access Only");
        showLCD("Mode: Door Only", "Ready");
      } else if (currentMode == 2) {
        Serial.println(">>> MODE: 2 - Time In");
        showLCD("Mode: Time In", "Light ON");
      } else if (currentMode == 3) {
        Serial.println(">>> MODE: 3 - Time Out");
        showLCD("Mode: Time Out", "Light OFF");
      }

      beepOnce(80);
      delay(1500);
    }
  }
}

void simulateDoorOpen() {
  Serial.println(">>> SIMULATION: DOOR OPEN");
  showLCD("Door Opening...", "");
  doorSimulatedOpen = true;
  delay(5000);
  Serial.println(">>> SIMULATION: DOOR CLOSE");
  doorSimulatedOpen = false;
}

void simulateEmergencyDoor() {
  if (doorSimulatedOpen) {
    Serial.println(">>> EMERGENCY: DOOR CLOSE");
    showLCD("Emergency:", "Door Closing...");
    doorSimulatedOpen = false;
  } else {
    Serial.println(">>> EMERGENCY: DOOR OPEN");
    showLCD("Emergency:", "Door Opening...");
    doorSimulatedOpen = true;
  }
  delay(1500);
  showLCD("Mode: Emergency", doorSimulatedOpen ? "DOOR OPEN" : "DOOR CLOSED");
}

String sendAttendance(String rfid, int mode) {
  WiFiClient client;
  HTTPClient http;

  String url = "http://" + String(SERVER_IP) + ":" + String(SERVER_PORT) + "/api/attendance";

  Serial.println("URL: " + url);
  Serial.println("Mode: " + String(mode));

  http.begin(client, url);
  http.addHeader("Content-Type", "application/json");

  String payload = "{\"rfid\":\"" + rfid + "\", \"mode\":" + String(mode) + "}";
  Serial.println("Payload: " + payload);

  int httpCode = http.POST(payload);
  Serial.println("HTTP Code: " + String(httpCode));

  String response = "";
  if (httpCode > 0) {
    response = http.getString();
  } else {
    Serial.println("HTTP POST failed");
  }

  http.end();
  return response;
}

bool parseResponse(String json, String& status, String& message, String& type, int& mode, String& door, String& light) {
  int statusIdx = json.indexOf("\"status\":");
  int messageIdx = json.indexOf("\"message\":");
  int typeIdx = json.indexOf("\"type\":");
  int modeIdx = json.indexOf("\"mode\":");
  int doorIdx = json.indexOf("\"door\":");
  int lightIdx = json.indexOf("\"light\":");

  if (statusIdx == -1) return false;

  int statusStart = json.indexOf("\"", statusIdx + 8) + 1;
  int statusEnd = json.indexOf("\"", statusStart);
  status = json.substring(statusStart, statusEnd);

  if (messageIdx != -1) {
    int msgStart = json.indexOf("\"", messageIdx + 10) + 1;
    int msgEnd = json.indexOf("\"", msgStart);
    message = json.substring(msgStart, msgEnd);
  }

  if (typeIdx != -1) {
    int typeStart = json.indexOf("\"", typeIdx + 7) + 1;
    int typeEnd = json.indexOf("\"", typeStart);
    type = json.substring(typeStart, typeEnd);
  }

  if (modeIdx != -1) {
    int modeStart = modeIdx + 7;
    int modeEnd = json.indexOf(",", modeStart);
    if (modeEnd == -1) modeEnd = json.indexOf("}", modeStart);
    String modeStr = json.substring(modeStart, modeEnd);
    modeStr.trim();
    mode = modeStr.toInt();
  }

  if (doorIdx != -1) {
    int doorStart = json.indexOf("\"", doorIdx + 7) + 1;
    int doorEnd = json.indexOf("\"", doorStart);
    door = json.substring(doorStart, doorEnd);
  }

  if (lightIdx != -1) {
    int lightStart = json.indexOf("\"", lightIdx + 8) + 1;
    int lightEnd = json.indexOf("\"", lightStart);
    light = json.substring(lightStart, lightEnd);
  }

  return true;
}

void setup() {
  Serial.begin(115200);

  pinMode(BUZZER_PIN, OUTPUT);
  buzzerOff();

  pinMode(RELAY_PIN, OUTPUT);
  relayLightOff();   // startup default OFF

  Wire.begin(21, 4);

  lcd.init();
  lcd.backlight();
  showLCD("Initializing...", "Please wait");

  SPI.begin(SCK_PIN, MISO_PIN, MOSI_PIN, SS_PIN);
  rfid.PCD_Init();

  connectWiFi();
  delay(1000);

  if (currentMode == 1) showLCD("Mode: Door Only", "Ready");
  else if (currentMode == 2) showLCD("Mode: Time In", "Light ON");
  else if (currentMode == 3) showLCD("Mode: Time Out", "Light OFF");

  Serial.println("Scan RFID tag");
}

void loop() {
  handleSerialCommands();

  if (!rfid.PICC_IsNewCardPresent()) return;
  if (!rfid.PICC_ReadCardSerial()) return;

  beepOnce(120);

  String uid = getUIDString();
  Serial.print("UID: ");
  Serial.println(uid);
  Serial.println("Current Mode: " + String(currentMode));

  showLCD("Card Detected", "Sending...");

  String response = sendAttendance(uid, currentMode);
  Serial.println("Server Response: " + response);

  String status = "";
  String message = "";
  String type = "";
  int mode = currentMode;
  String door = "no_change";
  String light = "no_change";

  if (parseResponse(response, status, message, type, mode, door, light)) {
    Serial.println("Status: " + status);
    Serial.println("Message: " + message);
    Serial.println("Door: " + door);
    Serial.println("Light: " + light);

    // Only apply action when access is successful
    if (status == "success") {
      if (door == "open") {
        simulateDoorOpen();
      }

      // ===== MAIN RELAY LOGIC =====
      if (currentMode == 2) {
        relayLightOn();   // TIME IN -> ON
      } else if (currentMode == 3) {
        relayLightOff();  // TIME OUT -> OFF
      }
      // Mode 1 -> no relay change

      beepTwiceShort();

      if (currentMode == 2) {
        showLCD("TIME IN OK", "Light ON");
      } else if (currentMode == 3) {
        showLCD("TIME OUT OK", "Light OFF");
      } else {
        showLCD("ACCESS GRANTED", message);
      }
    }
    else if (status == "denied") {
      beepLong();
      showLCD("ACCESS DENIED", message);
    }
    else if (status == "error") {
      beepLong();
      showLCD("SERVER ERROR", message);
    }
    else {
      beepLong();
      showLCD("Status: " + status, message);
    }
  } else {
    beepLong();
    showLCD("Connection Error", "Check Server");
  }

  delay(2000);

  if (currentMode == 1) showLCD("Mode: Door Only", "Ready");
  else if (currentMode == 2) showLCD("Mode: Time In", "Light ON");
  else if (currentMode == 3) showLCD("Mode: Time Out", "Light OFF");

  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();
}