#include <Wire.h>
#include <Adafruit_PN532.h>

#define SDA_PIN A4   // Pin SDA untuk PN532 (I2C)
#define SCL_PIN A5   // Pin SCL untuk PN532 (I2C)
#define BUZZER_PIN 8 // Pin untuk buzzer
#define LED_PIN 7    // Pin untuk LED

Adafruit_PN532 nfc(SDA_PIN, SCL_PIN);

// Data UID dari tiga kartu yang diizinkan (panjang UID tergantung kartu, biasanya 4-7 byte)
const uint8_t authorizedCards[3][7] = {
  {0xA, 0x92, 0x9C, 0x17}, // UID kartu 1
  {0xC9, 0x69, 0xE2, 0x6E}, // UID kartu 2
  {0xC1, 0x43, 0x30, 0x1D}  // UID kartu 3
};
const int numAuthorizedCards = 3;

void setup() {
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, HIGH); // LED menyala secara default
  digitalWrite(BUZZER_PIN, LOW); // Pastikan buzzer mati awalnya

  Serial.begin(9600);
  Serial.println("Initializing PN532...");

  nfc.begin();

  uint32_t versiondata = nfc.getFirmwareVersion();
  if (!versiondata) {
    Serial.println("PN532 not detected. Check connections.");
    while (1); // Berhenti di sini jika tidak ada modul PN532
  }

  // Cetak versi firmware
  Serial.print("Found PN532 with firmware version: ");
  Serial.println((versiondata >> 16) & 0xFF, HEX);
  Serial.println("Ready to scan RFID cards.");

  // Konfigurasikan modul PN532 untuk mode membaca kartu
  nfc.SAMConfig();
}

void loop() {
  uint8_t success;
  uint8_t uid[7];     // UID dari kartu
  uint8_t uidLength;

  // Periksa apakah ada kartu yang terbaca
  success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength);

  if (success) {
    Serial.println("RFID card detected!");

    // Cetak UID kartu ke serial monitor
    Serial.print("Card UID: ");
    for (uint8_t i = 0; i < uidLength; i++) {
      Serial.print(uid[i], HEX);
      Serial.print(" ");
    }
    Serial.println();

    // Periksa apakah kartu terdaftar
    if (isAuthorizedCard(uid, uidLength)) {
      Serial.println("Card is authorized!");
      // Bunyikan buzzer 2x dengan durasi 0,5 detik
      for (int i = 0; i < 2; i++) {
        digitalWrite(BUZZER_PIN, HIGH);
        digitalWrite(LED_PIN, LOW);
        delay(500);
        digitalWrite(BUZZER_PIN, LOW);
        digitalWrite(LED_PIN, HIGH);
        delay(500);
      }
    } else {
      Serial.println("Card is NOT authorized!");
      // Bunyikan buzzer 2x dengan durasi 2 detik
      for (int i = 0; i < 2; i++) {
        digitalWrite(BUZZER_PIN, HIGH);
        delay(2000);
        digitalWrite(BUZZER_PIN, LOW);
        delay(500);
      }
    }

    // Tunggu sebentar sebelum membaca kartu berikutnya
    delay(1000);
  }
}

// Fungsi untuk memeriksa apakah kartu diizinkan
bool isAuthorizedCard(uint8_t *uid, uint8_t length) {
  for (int i = 0; i < numAuthorizedCards; i++) {
    bool match = true;
    for (int j = 0; j < length; j++) {
      if (authorizedCards[i][j] != uid[j]) {
        match = false;
        break;
      }
    }
    if (match) {
      return true;
    }
  }
  return false;
}