/*
  RFID Door Lock
  ---------------
  Swings a servo bolt open when an authorized RFID card is scanned; flashes
  a red LED and beeps for unrecognized cards. Board: Arduino Nano.

  First run with LEARNING_MODE = true to discover your cards' UIDs via
  Serial Monitor, then paste them into AUTHORIZED_UIDS and set it to false.
*/

#include <SPI.h>
#include <MFRC522.h>
#include <Servo.h>

const uint8_t SS_PIN = 10;
const uint8_t RST_PIN = 9;
const uint8_t SERVO_PIN = 5;
const uint8_t GREEN_LED = 6;
const uint8_t RED_LED = 7;
const uint8_t BUZZER_PIN = 8;

const uint8_t LOCK_ANGLE = 0;
const uint8_t UNLOCK_ANGLE = 90;

bool LEARNING_MODE = true; // set false once AUTHORIZED_UIDS is filled in

// Each authorized card's 4-byte UID. Replace these placeholder examples
// with UIDs printed by the sketch while LEARNING_MODE is true.
const uint8_t AUTHORIZED_UIDS[][4] = {
  {0xDE, 0xAD, 0xBE, 0xEF},
  {0x12, 0x34, 0x56, 0x78},
};
const uint8_t NUM_AUTHORIZED = sizeof(AUTHORIZED_UIDS) / sizeof(AUTHORIZED_UIDS[0]);

MFRC522 rfid(SS_PIN, RST_PIN);
Servo lockServo;

void setup() {
  Serial.begin(9600);
  SPI.begin();
  rfid.PCD_Init();

  pinMode(GREEN_LED, OUTPUT);
  pinMode(RED_LED, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);

  lockServo.attach(SERVO_PIN);
  lockServo.write(LOCK_ANGLE);

  Serial.println(LEARNING_MODE
    ? "LEARNING MODE: scan a card to print its UID."
    : "Ready. Scan an authorized card.");
}

void loop() {
  if (!rfid.PICC_IsNewCardPresent() || !rfid.PICC_ReadCardSerial()) {
    return;
  }

  if (LEARNING_MODE) {
    printUid();
  } else if (isAuthorized()) {
    grantAccess();
  } else {
    denyAccess();
  }

  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();
}

void printUid() {
  Serial.print("Scanned UID: {");
  for (byte i = 0; i < rfid.uid.size; i++) {
    Serial.print("0x");
    if (rfid.uid.uidByte[i] < 0x10) Serial.print("0");
    Serial.print(rfid.uid.uidByte[i], HEX);
    if (i < rfid.uid.size - 1) Serial.print(", ");
  }
  Serial.println("}");
}

bool isAuthorized() {
  if (rfid.uid.size != 4) return false; // this sketch only compares 4-byte UIDs

  for (uint8_t i = 0; i < NUM_AUTHORIZED; i++) {
    bool match = true;
    for (uint8_t b = 0; b < 4; b++) {
      if (rfid.uid.uidByte[b] != AUTHORIZED_UIDS[i][b]) {
        match = false;
        break;
      }
    }
    if (match) return true;
  }
  return false;
}

void grantAccess() {
  Serial.println("Access granted.");
  digitalWrite(GREEN_LED, HIGH);
  lockServo.write(UNLOCK_ANGLE);
  delay(4000); // hold unlocked long enough to open the door
  lockServo.write(LOCK_ANGLE);
  digitalWrite(GREEN_LED, LOW);
}

void denyAccess() {
  Serial.println("Access denied.");
  for (uint8_t i = 0; i < 3; i++) {
    digitalWrite(RED_LED, HIGH);
    tone(BUZZER_PIN, 400, 150);
    delay(200);
    digitalWrite(RED_LED, LOW);
    delay(150);
  }
}
