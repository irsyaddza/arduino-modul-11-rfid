#include <Wire.h>
#include <Adafruit_PN532.h>
#include <LiquidCrystal_I2C.h>

#define SDA_PIN A4   // Pin SDA untuk PN532 (I2C)
#define SCL_PIN A5   // Pin SCL untuk PN532 (I2C)
#define LED_PIN 7    // Pin untuk LED

Adafruit_PN532 nfc(SDA_PIN, SCL_PIN);
LiquidCrystal_I2C lcd(0x27, 16, 2); // Alamat I2C untuk LCD (pastikan alamat sesuai dengan modul Anda)

// Data UID dan nama pemilik kartu
const uint8_t cardData[2][7] = {
  {0xB9, 0xD8, 0x53, 0x6E}, // UID kartu 1
  {0xC9, 0x69, 0xE2, 0x6E}  // UID Kartu 2
};
const char *cardOwners[2] = {
  "Rafael",  // Nama pemilik kartu 1
  "Irsyad"     // Nama pemilik kartu 2
};
const int numCards = 2;

void setup() {
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW); // Pastikan LED mati awalnya

  Serial.begin(9600);
  Serial.println("Initializing PN532 and LCD...");

  nfc.begin();
  lcd.init();
  lcd.backlight();

  uint32_t versiondata = nfc.getFirmwareVersion();
  if (!versiondata) {
    Serial.println("PN532 not detected. Check connections.");
    lcd.print("RFID ERROR");
    while (1); // Berhenti di sini jika tidak ada modul PN532
  }

  nfc.SAMConfig();
  lcd.clear();
  lcd.print("Ready to scan...");
}

void loop() {
  uint8_t success;
  uint8_t uid[7];     // UID dari kartu
  uint8_t uidLength;

  // Periksa apakah ada kartu yang terbaca
  success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength);

  if (success) {
    Serial.println("RFID card detected!");
    lcd.clear();

    // Cetak UID kartu ke serial monitor
    Serial.print("Card UID: ");
    for (uint8_t i = 0; i < uidLength; i++) {
      Serial.print(uid[i], HEX);
      Serial.print(" ");
    }
    Serial.println();

    // Periksa apakah kartu terdaftar
    int cardIndex = checkCard(uid, uidLength);
    if (cardIndex != -1) {
      Serial.println("Card is authorized!");
      lcd.print("Welcome, ");
      lcd.print(cardOwners[cardIndex]); // Tampilkan nama pemilik di LCD
    } else {
      Serial.println("Card is NOT authorized!");
      lcd.print("Access Denied");
      digitalWrite(LED_PIN, HIGH); // LED menyala
      delay(2000);
      digitalWrite(LED_PIN, LOW); // LED mati
    }

    // Tunggu sebentar sebelum membaca kartu berikutnya
    delay(2000);
    lcd.clear();
    lcd.print("Ready to scan...");
  }
}

// Fungsi untuk memeriksa kartu dan mengembalikan indeksnya jika terdaftar
int checkCard(uint8_t *uid, uint8_t length) {
  for (int i = 0; i < numCards; i++) {
    bool match = true;
    for (int j = 0; j < length; j++) {
      if (cardData[i][j] != uid[j]) {
        match = false;
        break;
      }
    }
    if (match) {
      return i; // Indeks kartu yang cocok
    }
  }
  return -1; // Kartu tidak ditemukan
}