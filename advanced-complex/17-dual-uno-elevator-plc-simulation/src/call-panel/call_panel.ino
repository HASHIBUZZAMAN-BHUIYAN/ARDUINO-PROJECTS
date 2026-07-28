/*
  Dual-Uno Elevator PLC Simulation - Call Panel (I2C master)
  -----------------------------------------------------------------
  Reads floor-call buttons, forwards calls to the car controller over
  I2C, and shows the car's polled state on a 7-segment display and
  direction LEDs. Board: Arduino Uno.
*/

#include <Wire.h>

const uint8_t CAR_ADDRESS = 0x20;
const uint8_t BUTTON_PINS[3] = {2, 3, 4};
const uint8_t SEGMENT_PINS[3] = {5, 6, 7}; // simplified 3-pin "digit select" for a common demo display
const uint8_t UP_LED_PIN = 8, DOWN_LED_PIN = 9;

enum State : uint8_t { IDLE = 0, MOVING_UP = 1, MOVING_DOWN = 2, DOOR_OPEN = 3 };

bool lastButtonState[3] = {HIGH, HIGH, HIGH};

unsigned long lastPollMs = 0;
const unsigned long POLL_INTERVAL_MS = 200; // 5Hz

void setup() {
  Serial.begin(9600);
  Wire.begin(); // master

  for (uint8_t i = 0; i < 3; i++) pinMode(BUTTON_PINS[i], INPUT_PULLUP);
  for (uint8_t i = 0; i < 3; i++) pinMode(SEGMENT_PINS[i], OUTPUT);
  pinMode(UP_LED_PIN, OUTPUT); pinMode(DOWN_LED_PIN, OUTPUT);

  Serial.println("Call panel ready.");
}

void loop() {
  checkButtons();

  unsigned long now = millis();
  if (now - lastPollMs >= POLL_INTERVAL_MS) {
    lastPollMs = now;
    pollCarState();
  }
}

void checkButtons() {
  for (uint8_t i = 0; i < 3; i++) {
    bool reading = digitalRead(BUTTON_PINS[i]);
    if (reading == LOW && lastButtonState[i] == HIGH) {
      sendCallRequest(i + 1);
    }
    lastButtonState[i] = reading;
  }
}

void sendCallRequest(uint8_t floor) {
  Wire.beginTransmission(CAR_ADDRESS);
  Wire.write(floor);
  Wire.endTransmission();
  Serial.print("Call requested: floor "); Serial.println(floor);
}

void pollCarState() {
  Wire.requestFrom(CAR_ADDRESS, (uint8_t)3);
  if (Wire.available() < 3) return;

  uint8_t currentFloor = Wire.read();
  uint8_t state = Wire.read();
  uint8_t doorOpen = Wire.read();

  displayFloor(currentFloor);
  digitalWrite(UP_LED_PIN, state == MOVING_UP ? HIGH : LOW);
  digitalWrite(DOWN_LED_PIN, state == MOVING_DOWN ? HIGH : LOW);

  Serial.print("Floor "); Serial.print(currentFloor);
  Serial.print(" state="); Serial.print(state);
  Serial.print(" door="); Serial.println(doorOpen);
}

void displayFloor(uint8_t floor) {
  // simplified: light one of 3 "digit select" pins per floor rather than
  // full 7-segment cathode/anode decoding, to keep the demo wiring minimal.
  for (uint8_t i = 0; i < 3; i++) {
    digitalWrite(SEGMENT_PINS[i], (i + 1) == floor ? HIGH : LOW);
  }
}
