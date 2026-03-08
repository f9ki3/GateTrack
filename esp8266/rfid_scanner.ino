#include <SPI.h>
#include <MFRC522.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

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

void setup() {
  Serial.begin(115200);

  // I2C: SDA=D2, SCL=D1
  Wire.begin(4, 5);

  lcd.init();
  lcd.backlight();
  showLCD("Initializing...", "Please wait");

  SPI.begin();       // SCK=D5, MISO=D6, MOSI=D7, SS=D8
  rfid.PCD_Init();

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

  showLCD("Card Detected", "Reading...");
  delay(800);

  showLCD("Access Scan OK", uid);
  delay(1500);

  showLCD("Scan RFID Card", "Ready");

  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();
}