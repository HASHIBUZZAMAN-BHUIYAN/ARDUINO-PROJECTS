/*
  RFID + Keypad Access Control Logger
  ------------------------------------------
  Two-factor door control (card + PIN, within a time window) with a full
  timestamped audit trail of every attempt logged to SD. Board: Arduino Mega 2560.

  First run with LEARNING_MODE = true to discover your cards' UIDs via
  Serial Monitor, then paste them into AUTHORIZED_UIDS and set it to false.
*/

#include <SPI.h>
#include <MFRC522.h>
#include <Keypad.h>
#include <Wire.h>
#include <RTClib.h>
#include <SD.h>
#include <LiquidCrystal.h>
#include <Servo.h>

const uint8_t RFID_SS_PIN = 48;
const uint8_t RFID_RST_PIN = 49;
const uint8_t SD_CS_PIN = 53;
const uint8_t SERVO_PIN = 9;
const uint8_t BUZZER_PIN = 8;

const char PIN_CODE[] = "4321"; // change this
const unsigned long PIN_WINDOW_MS = 10000;

bool LEARNING_MODE = true;
const uint8_t AUTHORIZED_UIDS[][4] = {
  {0xDE, 0xAD, 0xBE, 0xEF},
};
const uint8_t NUM_AUTHORIZED = sizeof(AUTHORIZED_UIDS) / sizeof(AUTHORIZED_UIDS[0]);

MFRC522 rfid(RFID_SS_PIN, RFID_RST_PIN);
RTC_DS3231 rtc;
LiquidCrystal lcd(22, 23, 24, 25, 26, 27);
Servo lockServo;

const uint8_t ROWS = 4, COLS = 4;
char keys[ROWS][COLS] = {
  {'1','2','3','A'},
  {'4','5','6','B'},
  {'7','8','9','C'},
  {'*','0','#','D'}
};
uint8_t rowPins[ROWS] = {A8, A9, A10, A11};
uint8_t colPins[COLS] = {A12, A13, A14, A15};
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

const uint8_t LOCK_ANGLE = 0;
const uint8_t UNLOCK_ANGLE = 90;
const char *LOG_FILENAME = "ACCESS.CSV";
bool sdReady = false;

enum State { IDLE, AWAITING_PIN };
State state = IDLE;
char lastUidStr[16];
char pinBuffer[5];
uint8_t pinLen = 0;
unsigned long pinWindowStart = 0;

void setup() {
  Serial.begin(9600);
  SPI.begin();
  rfid.PCD_Init();

  if (!rtc.begin()) Serial.println("RTC not found");
  sdReady = SD.begin(SD_CS_PIN);
  if (!sdReady) Serial.println("SD not found - logging disabled");
  else ensureLogHeader();

  lcd.begin(16, 2);
  lockServo.attach(SERVO_PIN);
  lockServo.write(LOCK_ANGLE);
  pinMode(BUZZER_PIN, OUTPUT);

  showIdleScreen();
}

void loop() {
  if (state == IDLE) {
    checkForCard();
  } else if (state == AWAITING_PIN) {
    checkPinEntry();
    if (millis() - pinWindowStart > PIN_WINDOW_MS) {
      logAttempt(lastUidStr, "TIMEOUT", "DENIED");
      denyFeedback();
      resetToIdle();
    }
  }
}

void checkForCard() {
  if (!rfid.PICC_IsNewCardPresent() || !rfid.PICC_ReadCardSerial()) return;

  uidToString(lastUidStr, sizeof(lastUidStr));

  if (LEARNING_MODE) {
    Serial.print("Scanned UID: ");
    Serial.println(lastUidStr);
    rfid.PICC_HaltA();
    return;
  }

  if (isAuthorizedCard()) {
    state = AWAITING_PIN;
    pinLen = 0;
    pinWindowStart = millis();
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Card OK. PIN?");
  } else {
    logAttempt(lastUidStr, "N/A", "DENIED (bad card)");
    denyFeedback();
  }

  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();
}

void checkPinEntry() {
  char key = keypad.getKey();
  if (!key) return;

  if (key == '#') {
    pinBuffer[pinLen] = '\0';
    bool correct = strcmp(pinBuffer, PIN_CODE) == 0;
    logAttempt(lastUidStr, pinBuffer, correct ? "GRANTED" : "DENIED (bad pin)");
    if (correct) grantAccess(); else denyFeedback();
    resetToIdle();
  } else if (key == '*') {
    pinLen = 0; // clear entry
  } else if (pinLen < 4) {
    pinBuffer[pinLen++] = key;
  }
}

bool isAuthorizedCard() {
  if (rfid.uid.size != 4) return false;
  for (uint8_t i = 0; i < NUM_AUTHORIZED; i++) {
    bool match = true;
    for (uint8_t b = 0; b < 4; b++) {
      if (rfid.uid.uidByte[b] != AUTHORIZED_UIDS[i][b]) { match = false; break; }
    }
    if (match) return true;
  }
  return false;
}

void grantAccess() {
  lcd.clear();
  lcd.print("Access granted");
  lockServo.write(UNLOCK_ANGLE);
  delay(4000);
  lockServo.write(LOCK_ANGLE);
}

void denyFeedback() {
  lcd.clear();
  lcd.print("Access denied");
  for (uint8_t i = 0; i < 3; i++) {
    tone(BUZZER_PIN, 400, 150);
    delay(250);
  }
}

void resetToIdle() {
  state = IDLE;
  showIdleScreen();
}

void showIdleScreen() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Scan card...");
}

void uidToString(char *out, size_t outSize) {
  out[0] = '\0';
  char byteStr[4];
  for (byte i = 0; i < rfid.uid.size; i++) {
    snprintf(byteStr, sizeof(byteStr), "%02X", rfid.uid.uidByte[i]);
    strncat(out, byteStr, outSize - strlen(out) - 1);
  }
}

void ensureLogHeader() {
  if (!SD.exists(LOG_FILENAME)) {
    File f = SD.open(LOG_FILENAME, FILE_WRITE);
    if (f) {
      f.println("timestamp,card_uid,pin_entered,result");
      f.close();
    }
  }
}

void logAttempt(const char *uid, const char *pin, const char *result) {
  Serial.print(uid); Serial.print(" | "); Serial.print(pin); Serial.print(" | "); Serial.println(result);

  if (!sdReady) return;
  File f = SD.open(LOG_FILENAME, FILE_WRITE);
  if (!f) return;

  DateTime now = rtc.now();
  char timestamp[20];
  snprintf(timestamp, sizeof(timestamp), "%04d-%02d-%02d %02d:%02d:%02d",
           now.year(), now.month(), now.day(), now.hour(), now.minute(), now.second());

  f.print(timestamp); f.print(",");
  f.print(uid); f.print(",");
  f.print(pin); f.print(",");
  f.println(result);
  f.close();
}
