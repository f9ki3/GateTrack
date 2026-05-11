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

// ================= Pins =================
#define BUZZER_PIN 27
#define RELAY_PIN  26
#define RELAY_ON   LOW
#define RELAY_OFF  HIGH

#define BTN_MODE1 33
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

// ================= State =================
bool doorIsOpen = false;
bool lightIsOn = false;
bool wifiConnected = false;
unsigned long lastWifiCheck = 0;
String lastCardUID = "";

unsigned long lastBtn1Press = 0;
unsigned long resetPressStartTime = 0;   // renamed for clarity
bool resetHandled = false;

#define DEBOUNCE_DELAY 180
#define LONG_PRESS_MS  3000
#define HTTP_TIMEOUT   8000

// ================= EEPROM Functions (unchanged) =================
void loadConfig() { /* ... same as previous ... */ }
void saveConfig() { /* ... same ... */ }
void clearConfig() { /* ... same ... */ }
void startConfigAP() { /* ... same ... */ }

// ================= LCD Helper =================
void showLCD(const String& line1, const String& line2 = "") {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(line1.substring(0, 16));
  lcd.setCursor(0, 1);
  lcd.print(line2.substring(0, 16));
}

// ================= Other Helpers (Buzzer, Door, Light, Stepper) =================
void beepOnce(int ms = 120) { digitalWrite(BUZZER_PIN, HIGH); delay(ms); digitalWrite(BUZZER_PIN, LOW); }
void beepTwice() { beepOnce(100); delay(100); beepOnce(100); }
void beepError() { beepOnce(150); delay(100); beepOnce(150); delay(100); beepOnce(150); }
void beepLongReset() { for(int i=0; i<3; i++){ beepOnce(200); delay(200); } }

void relayLightOn()  { digitalWrite(RELAY_PIN, RELAY_ON);  lightIsOn = true;  }
void relayLightOff() { digitalWrite(RELAY_PIN, RELAY_OFF); lightIsOn = false; }

void releaseStepper() { /* same */ }
void stepperOpenDoor() { /* same */ }
void stepperCloseDoor() { /* same */ }

void openDoor(String reason) {
  if (doorIsOpen) return;
  showLCD("Opening Door...", reason);
  beepOnce(100);
  stepperOpenDoor();
  doorIsOpen = true;
}

void closeDoor(String reason) {
  if (!doorIsOpen) return;
  showLCD("Closing Door...", reason);
  beepOnce(100);
  stepperCloseDoor();
  doorIsOpen = false;
}

void toggleDoor() {
  if (doorIsOpen) closeDoor("Manual");
  else openDoor("Manual");
  delay(600);
  showLCD("Manual Mode", doorIsOpen ? "Door OPEN" : "Door CLOSED");
}

// ================= HTTP =================
bool parseResponse(String json, String& status, String& message, String& type, String& userInfo) {
  status = message = type = userInfo = "";
  
  // Parse status
  int sIdx = json.indexOf("\"status\":\"");
  if (sIdx != -1) status = json.substring(sIdx+10, json.indexOf("\"", sIdx+10));
  
  // Parse type
  int tIdx = json.indexOf("\"type\":\"");
  if (tIdx != -1) type = json.substring(tIdx+8, json.indexOf("\"", tIdx+8));
  
  // Parse message
  int mIdx = json.indexOf("\"message\":\"");
  if (mIdx != -1) message = json.substring(mIdx+11, json.indexOf("\"", mIdx+11));
  
  // Parse user email
  int uIdx = json.indexOf("\"email\":\"");
  if (uIdx != -1) {
    String email = json.substring(uIdx+9, json.indexOf("\"", uIdx+9));
    userInfo = email.substring(0, email.indexOf('@')); // show username part
  }
  
  return status.length() > 0;
}

String sendRFID(String rfidTag, String& status, String& message, String& type, String& userInfo) {
  if (!checkWiFiConnection()) {
    status = "error";
    message = "No WiFi";
    return "";
  }

  WiFiClient client;
  HTTPClient http;
  String url = "http://" + savedServerIP + ":" + savedServerPort + "/api/attendance";
  String payload = "{\"rfid\":\"" + rfidTag + "\"}";

  http.begin(client, url);
  http.addHeader("Content-Type", "application/json");
  http.setTimeout(HTTP_TIMEOUT);
  
  int code = http.POST(payload);
  String response = http.getString();
  http.end();

  if (code > 0) parseResponse(response, status, message, type, userInfo);
  else {
    status = "error";
    message = "Server Error";
  }
  return response;
}

bool checkWiFiConnection() { /* same as before */ }

// ================= Button Handling =================
void handleButtons() { /* same as previous version */ }

// ================= Setup =================
void setup() {
  Serial.begin(115200);
  
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(RELAY_PIN, OUTPUT);
  relayLightOff();

  pinMode(BTN_MODE1, INPUT_PULLUP);
  pinMode(BTN_RESET, INPUT_PULLUP);

  // Stepper pins
  pinMode(STEPPER_IN1, OUTPUT); pinMode(STEPPER_IN2, OUTPUT);
  pinMode(STEPPER_IN3, OUTPUT); pinMode(STEPPER_IN4, OUTPUT);
  releaseStepper();
  doorStepper.setSpeed(STEPPER_SPEED_RPM);

  Wire.begin(21, 4);
  lcd.init();
  lcd.backlight();

  SPI.begin(SCK_PIN, MISO_PIN, MOSI_PIN, SS_PIN);
  rfid.PCD_Init();

  loadConfig();

  if (!wifiConfigured || savedSSID.length() == 0) {
    configMode = true;
    startConfigAP();
    // config server setup...
  } else {
    WiFi.mode(WIFI_STA);
    WiFi.begin(savedSSID.c_str(), savedWifiPass.c_str());
    checkWiFiConnection();
  }

  showLCD("GateTrack Ready", "Scan RFID or Btn1");
  Serial.println(">>> GateTrack Started - Improved LCD Messages");
}

// ================= Main Loop =================
void loop() {
  handleButtons();

  if (configMode) {
    configServer.handleClient();
    delay(10);
    return;
  }

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

  if (uid == lastCardUID) { delay(800); return; }

  lastCardUID = uid;
  showLCD("Card Detected", "Please wait...");

  String status = "", message = "", type = "", userInfo = "";
  sendRFID(uid, status, message, type, userInfo);

  if (status == "denied" || status == "error") {
    beepError();
    showLCD("ACCESS DENIED", message.length() > 0 ? message : "Invalid Card");
    delay(2500);
  } 
  else {
    // Show user info if available
    String topLine = userInfo.length() > 0 ? userInfo : "Success";
    
    if (type == "time_in") {
      relayLightOn();
      openDoor("Time In");
      delay(5000);
      closeDoor("Auto");
      beepTwice();
      showLCD("TIME IN RECORDED", topLine);
    }
    else if (type == "time_out") {
      openDoor("Time Out");
      delay(5000);
      closeDoor("Auto");
      relayLightOff();
      beepTwice();
      showLCD("TIME OUT RECORDED", topLine);
    }
    else {
      // Other success (already logged, access only, etc.)
      openDoor(message);
      delay(5000);
      closeDoor("Auto");
      beepTwice();
      showLCD(message.length() > 0 ? message.substring(0,16) : "ACCESS OK", topLine);
    }
  }

  // Wait for card removal
  unsigned long start = millis();
  while (millis() - start < 5000) {
    handleButtons();
    if (!rfid.PICC_IsNewCardPresent()) break;
    delay(100);
  }

  lastCardUID = "";
  rfid.PCD_Init();
  showLCD("GateTrack Ready", "Scan RFID or Btn1");
}