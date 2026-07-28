/*
  Two-Zone Security Alarm
  --------------------------
  Keypad-armed alarm covering a door reed switch (Zone 1) and a PIR sensor
  (Zone 2), with exit/entry delays before the siren sounds. Board: Arduino Uno.
*/

#include <Keypad.h>

const char CODE[] = "1234"; // change this to your own code

const uint8_t ROWS = 4, COLS = 4;
char keys[ROWS][COLS] = {
  {'1','2','3','A'},
  {'4','5','6','B'},
  {'7','8','9','C'},
  {'*','0','#','D'}
};
uint8_t rowPins[ROWS] = {2, 3, 4, 5};
uint8_t colPins[COLS] = {6, 7, 8, 9};
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

const uint8_t ZONE1_PIN = A0; // door reed switch
const uint8_t ZONE2_PIN = A1; // PIR motion
const uint8_t SIREN_PIN = 10;
const uint8_t RED_LED = 11;
const uint8_t GREEN_LED = 12;

const unsigned long EXIT_DELAY_MS = 5000;
const unsigned long ENTRY_DELAY_MS = 8000;

enum State { DISARMED, ARMING, ARMED, ENTRY_DELAY, TRIGGERED };
State state = DISARMED;

char inputBuffer[5]; // 4 digits + null terminator
uint8_t inputLen = 0;
unsigned long stateChangedAt = 0;

void setup() {
  pinMode(ZONE1_PIN, INPUT);
  pinMode(ZONE2_PIN, INPUT);
  pinMode(SIREN_PIN, OUTPUT);
  pinMode(RED_LED, OUTPUT);
  pinMode(GREEN_LED, OUTPUT);
  Serial.begin(9600);
  setState(DISARMED);
}

void loop() {
  char key = keypad.getKey();
  if (key) handleKey(key);

  switch (state) {
    case ARMING:
      if (millis() - stateChangedAt >= EXIT_DELAY_MS) setState(ARMED);
      break;

    case ARMED:
      if (zoneTripped()) {
        Serial.println("Zone tripped - entry delay started");
        setState(ENTRY_DELAY);
      }
      break;

    case ENTRY_DELAY:
      if (millis() - stateChangedAt >= ENTRY_DELAY_MS) {
        setState(TRIGGERED);
      }
      break;

    case TRIGGERED:
    case DISARMED:
      break; // handled entirely by handleKey() / zone checks above
  }
}

bool zoneTripped() {
  return digitalRead(ZONE1_PIN) == HIGH || digitalRead(ZONE2_PIN) == HIGH;
}

void handleKey(char key) {
  if (key == '#') {
    inputBuffer[inputLen] = '\0';
    checkCode();
    inputLen = 0;
    return;
  }
  if (key == '*') { // clear entry
    inputLen = 0;
    return;
  }
  if (inputLen < 4) {
    inputBuffer[inputLen++] = key;
  }
}

void checkCode() {
  if (strcmp(inputBuffer, CODE) != 0) {
    Serial.println("Wrong code.");
    return;
  }

  // Correct code: disarm from any state, or arm if currently disarmed.
  if (state == DISARMED) {
    Serial.println("Arming - exit now.");
    setState(ARMING);
  } else {
    Serial.println("Disarmed.");
    setState(DISARMED);
  }
}

void setState(State newState) {
  state = newState;
  stateChangedAt = millis();

  digitalWrite(RED_LED, (state != DISARMED) ? HIGH : LOW);
  digitalWrite(GREEN_LED, (state == DISARMED) ? HIGH : LOW);

  if (state == TRIGGERED) {
    tone(SIREN_PIN, 2500);
  } else {
    noTone(SIREN_PIN);
  }
}
