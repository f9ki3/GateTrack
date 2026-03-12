#include <SPI.h>
#include <MFRC522.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <HTTPClient.h>

// ================= WiFi Configuration =================
// UPDATE THESE TO YOUR WiFi CREDENTIALS
const char* WIFI_SSID = "Lleva";
const char* WIFI_PASSWORD = "4koSiFyke123*";

// ================= Flask Server Configuration =================
// UPDATE THIS TO YOUR SERVER IP
const char* SERVER_IP = "192.168.1.68";
const int SERVER_PORT = 5000;

// ================= Timeout Configuration =================
#define HTTP_TIMEOUT 8000       // 8 seconds timeout for HTTP requests

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
int currentMode = 1;
// 1 = Manual Button (door open until next scan - toggle)
// 2 = Time In  -> Light ON, Door Open, Close after 5s
// 3 = Time Out -> Light OFF, Door Open, Close after 5s

// ================= State Variables =================
bool doorIsOpen = false;           // For manual toggle mode
bool lightIsOn = false;
bool wifiConnected = false;
unsigned long lastWifiCheck = 0;
bool serverAvailable = true;

// Track last card UID to prevent double-scan
String lastCardUID = "";

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
    if (i < rfid.uid.size - 1) uid += " ";
  }
  uid.toUpperCase();
  return uid;
}

void showLCD(const String& line1, const String& line2 = "") {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(line1.substring(0, 16));
  if (line2.length() > 0) {
    lcd.setCursor(0, 1);
    lcd.print(line2.substring(0, 16));
  }
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

// ================= Relay / Light Functions =================
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

// ================= WiFi Functions =================
void connectWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Connecting WiFi");
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
    Serial.println("");
    Serial.println(">>> WiFi Connected!");
    Serial.print(">>> IP: ");
    Serial.println(WiFi.localIP());
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("WiFi Connected!");
    lcd.setCursor(0, 1);
    lcd.print(WiFi.localIP().toString().substring(0, 16));
    delay(2000);
  } else {
    wifiConnected = false;
    Serial.println("");
    Serial.println(">>> WiFi Failed!");
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("WiFi Failed!");
    delay(2000);
  }
}

bool checkWiFiConnection() {
  if (WiFi.status() == WL_CONNECTED) {
    if (!wifiConnected) {
      wifiConnected = true;
      Serial.println(">>> WiFi reconnected");
      showLCD("WiFi Reconnected", WiFi.localIP().toString().substring(0, 16));
      delay(1500);
    }
    return true;
  } else {
    wifiConnected = false;
    Serial.println(">>> WiFi disconnected, attempting reconnect...");
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
      showLCD("WiFi Reconnected", WiFi.localIP().toString().substring(0, 16));
      delay(1500);
      return true;
    } else {
      Serial.println(">>> WiFi reconnection failed");
      return false;
    }
  }
}

// ================= Door Functions =================
void openDoor(String reason) {
  Serial.println(">>> DOOR OPEN: " + reason);
  showLCD("Door Opening...", reason);
  doorIsOpen = true;
  beepOnce(100);
}

void closeDoor(String reason) {
  Serial.println(">>> DOOR CLOSE: " + reason);
  showLCD("Door Closing...", reason);
  doorIsOpen = false;
  beepOnce(100);
}

void toggleDoor() {
  if (doorIsOpen) {
    closeDoor("Manual Toggle");
  } else {
    openDoor("Manual Toggle");
  }
  delay(1500);
}

// ================= Show Current Mode =================
void showCurrentMode() {
  if (currentMode == 1) {
    if (doorIsOpen) {
      showLCD("Mode: Manual", "DOOR OPEN");
    } else {
      showLCD("Mode: Manual", "DOOR CLOSED");
    }
  } else if (currentMode == 2) {
    // Mode 2: Light OFF initially, will turn ON after successful RFID scan
    relayLightOff();
    showLCD("Mode: Time In", "Scan Card");
  } else if (currentMode == 3) {
    // Mode 3: Light ON initially (person leaving needs light), will turn OFF after successful RFID scan
    relayLightOn();
    showLCD("Mode: Time Out", "Scan Card");
  }
}

// ================= HTTP Functions =================
// Response parsing
bool parseResponse(String json, String& status, String& message) {
  // Check for valid JSON
  if (json.length() < 5) {
    return false;
  }
  
  // Find status field
  int statusIdx = json.indexOf("\"status\":\"");
  if (statusIdx == -1) {
    statusIdx = json.indexOf("\"status\": \"");
  }
  
  if (statusIdx == -1) {
    Serial.println(">>> No status in response");
    return false;
  }
  
  // Extract status value
  int statusStart = statusIdx + 10;
  int statusEnd = json.indexOf("\"", statusStart);
  if (statusEnd == -1) return false;
  
  status = json.substring(statusStart, statusEnd);
  
  // Find message field
  int msgIdx = json.indexOf("\"message\":\"");
  if (msgIdx == -1) {
    msgIdx = json.indexOf("\"message\": \"");
  }
  
  if (msgIdx != -1) {
    int msgStart = msgIdx + 10;
    int msgEnd = json.indexOf("\"", msgStart);
    if (msgEnd != -1) {
      message = json.substring(msgStart, msgEnd);
    }
  }
  
  Serial.println(">>> Parsed - Status: " + status + ", Message: " + message);
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

  Serial.println(">>> Sending to: " + url);
  Serial.println(">>> RFID: " + rfidTag + ", Mode: " + String(mode));

  http.begin(client, url);
  http.addHeader("Content-Type", "application/json");
  http.setTimeout(HTTP_TIMEOUT);

  String payload = "{\"rfid\":\"" + rfidTag + "\", \"mode\":" + String(mode) + "}";
  
  int httpCode = http.POST(payload);
  String response = "";

  if (httpCode > 0) {
    response = http.getString();
    Serial.println(">>> HTTP Response: " + String(httpCode));
    Serial.println(">>> Response Body: " + response);
    
    // Parse the response
    parseResponse(response, outStatus, outMessage);
  } else {
    Serial.println(">>> HTTP Error: " + String(httpCode) + " - " + http.errorToString(httpCode));
    outStatus = "error";
    outMessage = "HTTP " + String(httpCode);
    response = "{\"status\":\"error\",\"message\":\"HTTP " + String(httpCode) + "\"}";
  }

  http.end();
  return response;
}

// ================= Serial Commands =================
void handleSerialCommands() {
  if (Serial.available() > 0) {
    String command = Serial.readStringUntil('\n');
    command.trim();

    if (command == "r" || command == "R") {
      Serial.println(">>> MANUAL RESET");
      showLCD("Resetting...", "Please wait");
      delay(1000);
      ESP.restart();
      return;
    }

    if (command == "w" || command == "W") {
      Serial.println(">>> WiFi Reconnect");
      wifiConnected = false;
      checkWiFiConnection();
      return;
    }

    // Mode selection: 1, 2, 3
    if (command == "1" || command == "2" || command == "3") {
      currentMode = command.toInt();
      
      Serial.println(">>> MODE CHANGED TO: " + String(currentMode));
      
      if (currentMode == 1) {
        Serial.println(">>> Mode 1: Manual Toggle");
        showLCD("Mode: Manual", "Toggle Door");
        doorIsOpen = false;  // Reset door state
      } else if (currentMode == 2) {
        Serial.println(">>> Mode 2: Time In - Light ON after scan, Door Open 5s");
        relayLightOff();  // Light OFF initially
        showLCD("Mode: Time In", "Scan Card");
      } else if (currentMode == 3) {
        Serial.println(">>> Mode 3: Time Out - Light OFF after scan, Door Open 5s");
        relayLightOn();   // Light ON initially
        showLCD("Mode: Time Out", "Scan Card");
      }

      beepOnce(80);
      delay(1500);
    }
  }
}

// ================= Setup =================
void setup() {
  Serial.begin(115200);
  Serial.println("");
  Serial.println(">>> ==============================");
  Serial.println(">>> ESP32 RFID Scanner Starting");
  Serial.println(">>> ==============================");

  // Initialize pins
  pinMode(BUZZER_PIN, OUTPUT);
  buzzerOff();

  pinMode(RELAY_PIN, OUTPUT);
  relayLightOff();  // Start with light OFF

  // Initialize I2C for LCD
  Wire.begin(21, 4);

  lcd.init();
  lcd.backlight();
  showLCD("Initializing...", "Please wait");

  // Initialize SPI for RFID
  SPI.begin(SCK_PIN, MISO_PIN, MOSI_PIN, SS_PIN);
  rfid.PCD_Init();

  Serial.println(">>> RFID Reader initialized");

  // Connect to WiFi FIRST before anything else
  connectWiFi();

  // Show initial mode (lights will be controlled after RFID scan)
  if (currentMode == 1) {
    showLCD("Mode: Manual", "Toggle Door");
  } else if (currentMode == 2) {
    relayLightOff();  // Light OFF initially for Time In
    showLCD("Mode: Time In", "Scan Card");
  } else if (currentMode == 3) {
    relayLightOn();   // Light ON initially for Time Out
    showLCD("Mode: Time Out", "Scan Card");
  }

  Serial.println("");
  Serial.println(">>> READY - Scan RFID card");
  Serial.println(">>> Commands: 1=Manual, 2=TimeIn, 3=TimeOut, W=WiFi, R=Reset");
}

// ================= Main Loop =================
void loop() {
  // Handle serial commands from user
  handleSerialCommands();

  // Periodic WiFi health check (every 60 seconds)
  unsigned long currentMillis = millis();
  if (currentMillis - lastWifiCheck > 60000) {
    lastWifiCheck = currentMillis;
    checkWiFiConnection();
  }

  // Check for new RFID card
  if (!rfid.PICC_IsNewCardPresent()) return;
  if (!rfid.PICC_ReadCardSerial()) return;

  // Card detected - beep
  beepOnce(120);
  
  // Get RFID UID
  String uid = getUIDString();
  Serial.println("");
  Serial.println(">>> ========== CARD DETECTED ==========");
  Serial.println(">>> UID: " + uid);
  Serial.println(">>> Mode: " + String(currentMode));

  // Prevent double-scan: wait for card to be removed
  if (uid == lastCardUID) {
    Serial.println(">>> Same card detected, waiting for removal...");
    showLCD("Wait...", "Remove card first");
    
    // Wait until card is removed
    while (rfid.PICC_IsNewCardPresent() || rfid.PICC_ReadCardSerial()) {
      delay(100);
      rfid.PICC_HaltA();
      rfid.PCD_StopCrypto1();
    }
    
    delay(500);
    reinitRFID();
    return;
  }
  
  lastCardUID = uid;
  showLCD("Card Detected", "Processing...");

  // ================= MODE 1: MANUAL BUTTON =================
  if (currentMode == 1) {
    // Manual button - immediate toggle door open/close (no server call)
    // Act like a button: tap to open, tap again to close
    toggleDoor();

    // Show status
    if (doorIsOpen) {
      showLCD("DOOR OPEN", "Tap to close");
    } else {
      showLCD("DOOR CLOSED", "Tap to open");
    }
    
    beepTwice();
    delay(2000);
  }

  // ================= MODE 2: TIME IN =================
  else if (currentMode == 2) {
    // Send attendance to server FIRST
    String status = "";
    String message = "";
    String response = sendAttendance(uid, 2, status, message);
    Serial.println(">>> Server Response: " + response);

    // Check if access denied or error
    if (status == "denied") {
      beepError();
      showLCD("ACCESS DENIED", message);
      delay(2500);
      
      // Reset for next scan
      lastCardUID = "";
      showCurrentMode();
      return;
    }

    if (status != "success") {
      showLCD("ERROR", message);
      delay(2000);
      
      // Reset for next scan
      lastCardUID = "";
      showCurrentMode();
      return;
    }

    // Only turn ON light AFTER successful log
    relayLightOn();
    lightIsOn = true;
    Serial.println(">>> LIGHT SET TO: ON (after success)");
    
    // Open door
    openDoor("Time In");
    
    // Wait 5 seconds then close door
    delay(5000);
    closeDoor("Auto Close");
    
    // Success feedback
    beepTwice();
    showLCD("TIME IN OK", "Light ON");
    
    delay(2000);
  }

  // ================= MODE 3: TIME OUT =================
  else if (currentMode == 3) {
    // Send attendance to server FIRST
    String status = "";
    String message = "";
    String response = sendAttendance(uid, 3, status, message);
    Serial.println(">>> Server Response: " + response);

    // Check if access denied or error
    if (status == "denied") {
      beepError();
      showLCD("ACCESS DENIED", message);
      delay(2500);
      
      // Reset for next scan
      lastCardUID = "";
      showCurrentMode();
      return;
    }

    if (status != "success") {
      showLCD("ERROR", message);
      delay(2000);
      
      // Reset for next scan
      lastCardUID = "";
      showCurrentMode();
      return;
    }

    // Only turn OFF light AFTER successful log
    relayLightOff();
    lightIsOn = false;
    Serial.println(">>> LIGHT SET TO: OFF (after success)");
    
    // Open door
    openDoor("Time Out");
    
    // Wait 5 seconds then close door
    delay(5000);
    closeDoor("Auto Close");
    
    // Success feedback
    beepTwice();
    showLCD("TIME OUT OK", "Light OFF");
    
    delay(2000);
  }

  // Wait for card to be removed before allowing next scan
  Serial.println(">>> Waiting for card removal...");
  showLCD("Done!", "Remove card");
  
  // Wait until card is removed
  while (rfid.PICC_IsNewCardPresent() || rfid.PICC_ReadCardSerial()) {
    delay(100);
  }
  
  // Re-initialize RFID reader for next scan
  delay(200);
  reinitRFID();
  lastCardUID = "";  // Reset last card UID

  // Show current mode status
  if (currentMode == 1) {
    if (doorIsOpen) {
      showLCD("Mode: Manual", "DOOR OPEN");
    } else {
      showLCD("Mode: Manual", "DOOR CLOSED");
    }
  } else if (currentMode == 2) {
    // Light will be ON after successful scan (stay on until mode changes)
    showLCD("Mode: Time In", "Scan Card");
  } else if (currentMode == 3) {
    // Light will be OFF after successful scan (stay off until mode changes)
    showLCD("Mode: Time Out", "Scan Card");
  }
}

