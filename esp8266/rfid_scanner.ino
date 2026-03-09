#include <SPI.h>
#include <MFRC522.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266HTTPClient.h>

// ================= WiFi Configuration =================
const char* WIFI_SSID = "Lleva";
const char* WIFI_PASSWORD = "4koSiFyke123*";

// ================= Flask Server Configuration =================
const char* SERVER_IP = "192.168.1.100";  
const int SERVER_PORT = 5000;

// ================= RC522 =================
#define SS_PIN 15   // D8
#define RST_PIN 0   // D3

MFRC522 rfid(SS_PIN, RST_PIN);

// ================= LCD I2C =================
LiquidCrystal_I2C lcd(0x27, 16, 2);

// ================= Mode Configuration =================
int currentMode = 2;  // Default mode: 2 (Time In)
// Modes: 1=Door Only, 2=Time In, 3=Time Out

// ================= Simulation Variables =================
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

void connectWiFi() {
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Connecting WiFi");
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
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

// Handle serial commands
// 1, 2, 3 = set mode for RFID scanning
// 4 = IMMEDIATELY toggle door without RFID
void handleSerialCommands() {
  if (Serial.available() > 0) {
    String command = Serial.readStringUntil('\n');
    command.trim();
    
    // Mode 4 - IMMEDIATELY toggle door without RFID scan
    if (command == "4") {
      Serial.println(">>> EMERGENCY: Toggling door NOW");
      simulateEmergencyDoor();  // Immediate door toggle
      return;
    }
    
    // Mode 1, 2, 3 - set mode for RFID scanning
    if (command == "1" || command == "2" || command == "3") {
      int mode = command.toInt();
      currentMode = mode;
      
      if (mode == 1) {
        Serial.println(">>> MODE: 1 - Door Access Only");
        showLCD("Mode: Door Only", "Ready");
      } else if (mode == 2) {
        Serial.println(">>> MODE: 2 - Time In + Door + Light ON");
        showLCD("Mode: Time In", "Light ON - Ready");
      } else if (mode == 3) {
        Serial.println(">>> MODE: 3 - Time Out + Door + Light OFF");
        showLCD("Mode: Time Out", "Light OFF - Ready");
      }
      delay(1500);
    } else if (command == "STATUS" || command == "S") {
      Serial.println("Current Mode: " + String(currentMode));
    } else if (command == "HELP" || command == "H") {
      Serial.println("=== Commands ===");
      Serial.println("1   - Set mode: Door Only");
      Serial.println("2   - Set mode: Time In + Light ON");
      Serial.println("3   - Set mode: Time Out + Light OFF");
      Serial.println("4   - EMERGENCY: Toggle door NOW");
      Serial.println("S   - Show current mode");
      Serial.println("H   - Show help");
    }
    delay(100);
  }
}

// SIMULATION: Door control
void simulateDoorOpen() {
  Serial.println(">>> SIMULATION: DOOR OPEN");
  showLCD("Door Opening...", "");
  doorSimulatedOpen = true;
  delay(5000);
  Serial.println(">>> SIMULATION: DOOR CLOSE");
  doorSimulatedOpen = false;
}

// SIMULATION: Light control
void simulateLightOn() {
  Serial.println(">>> SIMULATION: LIGHT ON");
  lightSimulatedOn = true;
}

void simulateLightOff() {
  Serial.println(">>> SIMULATION: LIGHT OFF");
  lightSimulatedOn = false;
}

// SIMULATION: Emergency door toggle - open/close without delay
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

// Send RFID to server with mode and get response
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
    Serial.println("HTTP Error: " + http.errorToString(httpCode));
  }
  
  http.end();
  return response;
}

// Parse JSON response
bool parseResponse(String json, String& status, String& message, String& type, int& mode, String& door, String& light) {
  int statusIdx = json.indexOf("\"status\":");
  int messageIdx = json.indexOf("\"message\":");
  int typeIdx = json.indexOf("\"type\":");
  int modeIdx = json.indexOf("\"mode\":");
  int doorIdx = json.indexOf("\"door\":");
  int lightIdx = json.indexOf("\"light\":");
  
  if (statusIdx == -1) return false;
  
  // Extract status
  int statusStart = json.indexOf("\"", statusIdx + 8) + 1;
  int statusEnd = json.indexOf("\"", statusStart);
  status = json.substring(statusStart, statusEnd);
  
  // Extract message
  if (messageIdx != -1) {
    int msgStart = json.indexOf("\"", messageIdx + 9) + 1;
    int msgEnd = json.indexOf("\"", msgStart);
    message = json.substring(msgStart, msgEnd);
  }
  
  // Extract type
  if (typeIdx != -1) {
    int typeStart = json.indexOf("\"", typeIdx + 6) + 1;
    int typeEnd = json.indexOf("\"", typeStart);
    type = json.substring(typeStart, typeEnd);
  }
  
  // Extract mode
  if (modeIdx != -1) {
    int modeStart = modeIdx + 6;
    int modeEnd = json.indexOf(",", modeStart);
    if (modeEnd == -1) modeEnd = json.indexOf("}", modeStart);
    String modeStr = json.substring(modeStart, modeEnd);
    modeStr.trim();
    mode = modeStr.toInt();
  }
  
  // Extract door
  if (doorIdx != -1) {
    int doorStart = json.indexOf("\"", doorIdx + 6) + 1;
    int doorEnd = json.indexOf("\"", doorStart);
    door = json.substring(doorStart, doorEnd);
  }
  
  // Extract light
  if (lightIdx != -1) {
    int lightStart = json.indexOf("\"", lightIdx + 7) + 1;
    int lightEnd = json.indexOf("\"", lightStart);
    light = json.substring(lightStart, lightEnd);
  }
  
  return true;
}

void setup() {
  Serial.begin(115200);
  Serial.println("=== RFID Scanner Starting ===");
  Serial.println("1 = Door Only mode");
  Serial.println("2 = Time In mode");
  Serial.println("3 = Time Out mode");
  Serial.println("4 = EMERGENCY toggle door NOW");
  Serial.println("");
  Serial.println(">>> MODE: 2 - Time In + Light ON");

  Wire.begin(4, 5);  // I2C: SDA=D2, SCL=D1

  lcd.init();
  lcd.backlight();
  showLCD("Initializing...", "Please wait");

  SPI.begin();
  rfid.PCD_Init();

  connectWiFi();
  delay(1000);
  
  if (currentMode == 1) showLCD("Mode: Door Only", "Ready");
  else if (currentMode == 2) showLCD("Mode: Time In", "Light ON - Ready");
  else if (currentMode == 3) showLCD("Mode: Time Out", "Light OFF - Ready");
  
  Serial.println("Scan RFID tag");
}

void loop() {
  // Handle serial commands first
  handleSerialCommands();
  
  // Check for RFID card
  if (!rfid.PICC_IsNewCardPresent()) return;
  if (!rfid.PICC_ReadCardSerial()) return;

  String uid = getUIDString();
  Serial.print("UID: ");
  Serial.println(uid);
  Serial.println("Current Mode: " + String(currentMode));

  showLCD("Card Detected", "Sending...");

  // Send to server
  String response = sendAttendance(uid, currentMode);
  Serial.println("Server Response: " + response);

  // Parse response
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
    
    if (status == "success" || status == "denied") {
      if (door == "open") {
        simulateDoorOpen();
        
        if (light == "on") {
          simulateLightOn();
          showLCD("ACCESS GRANTED", "Light ON");
        } else if (light == "off") {
          simulateLightOff();
          showLCD("ACCESS GRANTED", "Light OFF");
        } else {
          showLCD("ACCESS GRANTED", message);
        }
      } else {
        if (status == "denied") {
          showLCD("ACCESS DENIED", message);
        } else {
          showLCD(message, "");
        }
      }
    } else if (status == "error") {
      showLCD("SERVER ERROR", message);
    } else {
      showLCD("Status: " + status, message);
    }
  } else {
    showLCD("Connection Error", "Check Server");
  }
  
  delay(2000);
  
  if (currentMode == 1) showLCD("Mode: Door Only", "Ready");
  else if (currentMode == 2) showLCD("Mode: Time In", "Light ON - Ready");
  else if (currentMode == 3) showLCD("Mode: Time Out", "Light OFF - Ready");

  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();
}

