#include <SPI.h>
#include <MFRC522.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266HTTPClient.h>

// ================= WiFi Configuration =================
// CHANGE THESE TO YOUR WIFI CREDENTIALS
const char* WIFI_SSID = "Lleva";
const char* WIFI_PASSWORD = "4koSiFyke123*";

// ================= Flask Server Configuration =================
// CHANGE THIS TO YOUR FLASK SERVER IP ADDRESS
const char* SERVER_IP = "192.168.1.68";  
const int SERVER_PORT = 5000;

// ================= RC522 =================
#define SS_PIN 15   // D8
#define RST_PIN 0   // D3

MFRC522 rfid(SS_PIN, RST_PIN);

// ================= LCD I2C =================
// Common addresses: 0x27 or 0x3F
LiquidCrystal_I2C lcd(0x27, 16, 2);

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

// Send RFID to server and get response
String sendAttendance(String rfid) {
  WiFiClient client;
  HTTPClient http;
  
  String url = "http://" + String(SERVER_IP) + ":" + String(SERVER_PORT) + "/api/attendance";
  
  Serial.println("URL: " + url);
  
  http.begin(client, url);
  http.addHeader("Content-Type", "application/json");
  
  String payload = "{\"rfid\":\"" + rfid + "\"}";
  
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

// Parse JSON response (simple parser for status and message)
bool parseResponse(String json, String& status, String& message, String& type) {
  // Simple string parsing - look for "status": and "message":
  int statusIdx = json.indexOf("\"status\":");
  int messageIdx = json.indexOf("\"message\":");
  int typeIdx = json.indexOf("\"type\":");
  
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
  
  // Extract type (time_in, time_out, already_logged, denied)
  if (typeIdx != -1) {
    int typeStart = json.indexOf("\"", typeIdx + 6) + 1;
    int typeEnd = json.indexOf("\"", typeStart);
    type = json.substring(typeStart, typeEnd);
  }
  
  return true;
}

void setup() {
  Serial.begin(115200);

  // I2C: SDA=D2, SCL=D1
  Wire.begin(4, 5);

  lcd.init();
  lcd.backlight();
  showLCD("Initializing...", "Please wait");

  SPI.begin();       // SCK=D5, MISO=D6, MOSI=D7, SS=D8
  rfid.PCD_Init();

  // Connect to WiFi
  connectWiFi();

  delay(1000);
  showLCD("Scan RFID Card", "Ready");
  Serial.println("Scan RFID tag");
}

void loop() {
  if (!rfid.PICC_IsNewCardPresent()) return;
  if (!rfid.PICC_ReadCardSerial()) return;

  String uid = getUIDString();

  Serial.print("UID: ");
  Serial.println(uid);

  showLCD("Card Detected", "Sending...");

  // Send to server
  String response = sendAttendance(uid);
  
  Serial.println("Server Response: " + response);

  // Parse response
  String status = "";
  String message = "";
  String type = "";
  
  if (parseResponse(response, status, message, type)) {
    Serial.println("Status: " + status);
    Serial.println("Message: " + message);
    Serial.println("Type: " + type);
    
    if (status == "success") {
      if (type == "time_in") {
        showLCD("ACCESS GRANTED", "Time In Recorded");
        Serial.println("Time In Recorded");
      } else if (type == "time_out") {
        showLCD("ACCESS GRANTED", "Time Out Recorded");
        Serial.println("Time Out Recorded");
      } else if (type == "already_logged") {
        showLCD("ALREADY LOGGED", "Today Complete");
        Serial.println("Already logged today");
      } else {
        showLCD("ACCESS GRANTED", message);
      }
    } else if (status == "denied") {
      showLCD("ACCESS DENIED", message);
      Serial.println("Access Denied: " + message);
    } else if (status == "error") {
      showLCD("SERVER ERROR", message);
      Serial.println("Server Error: " + message);
    } else {
      showLCD("Status: " + status, message);
    }
  } else {
    // Failed to parse or connect
    showLCD("Connection Error", "Check Server");
    Serial.println("Failed to parse response or no connection");
    Serial.println("Raw response: " + response);
  }

  delay(2000);
  showLCD("Scan RFID Card", "Ready");

  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();
}

