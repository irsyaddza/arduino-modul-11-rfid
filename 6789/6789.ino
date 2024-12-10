#include <LiquidCrystal_I2C.h>
#include <Wire.h>
#include <PN532_I2C.h>
#include <PN532.h>
#include <NfcAdapter.h>
#include <EEPROM.h>

PN532_I2C pn532_i2c(Wire);
NfcAdapter nfc = NfcAdapter(pn532_i2c);

// Initialize LCD (I2C address: 0x27, 16 columns, 2 rows)
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Pin definitions
const int BUZZER_PIN = 8;
const int LED_PIN = 6;    // Must be PWM pin
const int BUTTON_PIN = 2; // Using interrupt-capable pin

// RFID card data
byte savedUID[7] = {0xB9, 0xD8, 0x53, 0x6E, 0x0E, 0x00, 0x08}; // Example UID

// System states
enum SystemState {
    CASE_A,
    CASE_B,
    CASE_C,
    SYSTEM_OFF
};

volatile SystemState currentState = CASE_A;
volatile unsigned long lastButtonPress = 0;
volatile bool buttonPressed = false;
const unsigned long DEBOUNCE_DELAY = 250;

// Function declarations
void handleButtonPress();
void fadeLED();
void blinkLED(int times, int duration);
void displayRFIDData(byte* uid);

void setup() {
    Serial.begin(9600);
    
    // Initialize LCD
    lcd.init();
    lcd.backlight();
    
    // Initialize RFID
    nfc.begin();
    
    // Initialize pins
    pinMode(BUZZER_PIN, OUTPUT);
    pinMode(LED_PIN, OUTPUT);
    pinMode(BUTTON_PIN, INPUT_PULLUP);
    
    // Attach interrupt for button
    attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), handleButtonPress, FALLING);
    
    // Initial display
    lcd.clear();
    lcd.print("RFID System");
    lcd.setCursor(0, 1);
    lcd.print("Mode: Case A");
    
    Serial.println("System initialized!");
}

void handleButtonPress() {
    unsigned long currentTime = millis();
    if (currentTime - lastButtonPress > DEBOUNCE_DELAY) {
        buttonPressed = true;
        lastButtonPress = currentTime;
        
        // Update system state
        switch (currentState) {
            case CASE_A:
                currentState = CASE_B;
                break;
            case CASE_B:
                currentState = CASE_C;
                break;
            case CASE_C:
                currentState = CASE_A;
                break;
            case SYSTEM_OFF:
                currentState = CASE_A;
                break;
        }
    }
}

void fadeLED() {
    for (int i = 0; i < 3; i++) {  // 3 times fade in/out
        // Fade in
        for (int brightness = 0; brightness <= 255; brightness++) {
            analogWrite(LED_PIN, brightness);
            delay(2);
        }
        // Fade out
        for (int brightness = 255; brightness >= 0; brightness--) {
            analogWrite(LED_PIN, brightness);
            delay(2);
        }
    }
    analogWrite(LED_PIN, 0);  // Ensure LED is off at end
}

void blinkLED(int times, int duration) {
    for (int i = 0; i < times; i++) {
        digitalWrite(LED_PIN, HIGH);
        delay(duration);
        digitalWrite(LED_PIN, LOW);
        delay(duration);
    }
}

void displayRFIDData(byte* uid) {
    lcd.clear();
    lcd.print("Card detected:");
    lcd.setCursor(0, 1);
    
    // Display UID on LCD
    for (int i = 0; i < 4; i++) {  // Show first 4 bytes only due to LCD space
        if (uid[i] < 0x10) lcd.print("0");
        lcd.print(String(uid[i], HEX));
    }
    
    // Display full UID on Serial
    Serial.print("Card UID: ");
    for (int i = 0; i < 7; i++) {
        if (uid[i] < 0x10) Serial.print("0");
        Serial.print(uid[i], HEX);
        Serial.print(" ");
    }
    Serial.println();
}

bool compareUID(byte* uid1, byte* uid2) {
    return memcmp(uid1, uid2, 7) == 0;
}

void handleCaseA(byte* uid) {
    displayRFIDData(uid);
    if (compareUID(uid, savedUID)) {
        // Single beep
        digitalWrite(BUZZER_PIN, HIGH);
        delay(200);
        digitalWrite(BUZZER_PIN, LOW);
        
        // Fade LED 3 times
        fadeLED();
    }
}

void handleCaseB(byte* uid) {
    displayRFIDData(uid);
    if (compareUID(uid, savedUID)) {
        // Blink LED and Buzzer 10 times
        for (int i = 0; i < 10; i++) {
            digitalWrite(BUZZER_PIN, HIGH);
            digitalWrite(LED_PIN, HIGH);
            delay(100);  // Adjust delay as needed for blink duration
            digitalWrite(BUZZER_PIN, LOW);
            digitalWrite(LED_PIN, LOW);
            delay(100);  // Adjust delay between blinks
        }
    }
}

void handleCaseC() {
    lcd.clear();
    lcd.print("System OFF");
    lcd.setCursor(0, 1);
    lcd.print("Press to restart");
    
    while (currentState == CASE_C) {
        blinkLED(1, 500);  // Continuous blinking until button press
    }
}

void updateDisplayState() {
    lcd.clear();
    lcd.print("RFID System");
    lcd.setCursor(0, 1);
    switch (currentState) {
        case CASE_A:
            lcd.print("Mode: Case A");
            break;
        case CASE_B:
            lcd.print("Mode: Case B");
            break;
        case CASE_C:
            lcd.print("Mode: Case C");
            break;
    }
}

void loop() {
    // Handle button press state change
    if (buttonPressed) {
        updateDisplayState();
        buttonPressed = false;
        delay(100);  // Small delay to prevent bouncing
    }
    
    // Main state machine
    switch (currentState) {
        case CASE_A:
        case CASE_B:
            if (nfc.tagPresent()) {
                NfcTag tag = nfc.read();
                byte uid[7];
                tag.getUid(uid, sizeof(uid));
                
                if (currentState == CASE_A) {
                    handleCaseA(uid);
                } else {
                    handleCaseB(uid);
                }
                delay(1000);  // Prevent multiple reads
            }
            break;
            
        case CASE_C:
            handleCaseC();
            break;
            
        default:
            break;
    }
}