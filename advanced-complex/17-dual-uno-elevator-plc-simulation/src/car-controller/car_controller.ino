/*
  Dual-Uno Elevator PLC Simulation - Car Controller (I2C slave 0x20)
  ---------------------------------------------------------------------
  Owns the elevator's state machine and closed-loop floor positioning
  against physical limit switches, with a simplified elevator-algorithm
  call queue. Board: Arduino Uno.
*/

#include <Wire.h>

const uint8_t I2C_ADDRESS = 0x20;
const uint8_t LIMIT_PINS[3] = {2, 3, 4}; // floor 1, 2, 3 (index 0,1,2)
const uint8_t MOTOR_UP_PIN = 5, MOTOR_DOWN_PIN = 6, DOOR_LED_PIN = 7;

enum State : uint8_t { IDLE = 0, MOVING_UP = 1, MOVING_DOWN = 2, DOOR_OPEN = 3 };

volatile uint8_t currentFloor = 1;
volatile uint8_t state = IDLE;
volatile bool doorOpenFlag = false;
volatile uint8_t requestedFloor = 0; // set via I2C write, 0 = none pending

bool pendingCalls[3] = {false, false, false};
int8_t travelDirection = 1; // 1 = up, -1 = down (for SCAN-like ordering)

unsigned long doorOpenedAtMs = 0;
const unsigned long DOOR_DWELL_MS = 3000;

void setup() {
  Serial.begin(9600);
  for (uint8_t i = 0; i < 3; i++) pinMode(LIMIT_PINS[i], INPUT_PULLUP);
  pinMode(MOTOR_UP_PIN, OUTPUT); pinMode(MOTOR_DOWN_PIN, OUTPUT); pinMode(DOOR_LED_PIN, OUTPUT);
  digitalWrite(MOTOR_UP_PIN, LOW); digitalWrite(MOTOR_DOWN_PIN, LOW); digitalWrite(DOOR_LED_PIN, LOW);

  Wire.begin(I2C_ADDRESS);
  Wire.onRequest(onRequest);
  Wire.onReceive(onReceive);

  currentFloor = detectFloorFromSwitches();
  Serial.println("Car controller ready.");
}

void loop() {
  switch (state) {
    case IDLE:
      handleIdle();
      break;
    case MOVING_UP:
    case MOVING_DOWN:
      handleMoving();
      break;
    case DOOR_OPEN:
      handleDoorOpen();
      break;
  }

  if (requestedFloor != 0) {
    uint8_t f = requestedFloor;
    requestedFloor = 0;
    if (f >= 1 && f <= 3) pendingCalls[f - 1] = true;
  }
}

uint8_t detectFloorFromSwitches() {
  for (uint8_t i = 0; i < 3; i++) {
    if (digitalRead(LIMIT_PINS[i]) == LOW) return i + 1;
  }
  return 1; // default assumption if none triggered (e.g. between floors)
}

void handleIdle() {
  int target = pickNextTarget();
  if (target == 0) return; // no pending calls

  if (target > currentFloor) {
    state = MOVING_UP;
    digitalWrite(MOTOR_UP_PIN, HIGH);
    travelDirection = 1;
  } else if (target < currentFloor) {
    state = MOVING_DOWN;
    digitalWrite(MOTOR_DOWN_PIN, HIGH);
    travelDirection = -1;
  } else {
    pendingCalls[target - 1] = false;
    openDoor();
  }
}

int pickNextTarget() {
  // SCAN-like: prefer a pending call in the current direction of travel first.
  if (travelDirection > 0) {
    for (int f = currentFloor + 1; f <= 3; f++) if (pendingCalls[f - 1]) return f;
    for (int f = currentFloor - 1; f >= 1; f--) if (pendingCalls[f - 1]) return f;
  } else {
    for (int f = currentFloor - 1; f >= 1; f--) if (pendingCalls[f - 1]) return f;
    for (int f = currentFloor + 1; f <= 3; f++) if (pendingCalls[f - 1]) return f;
  }
  if (pendingCalls[currentFloor - 1]) return currentFloor;
  return 0;
}

void handleMoving() {
  for (uint8_t i = 0; i < 3; i++) {
    if (digitalRead(LIMIT_PINS[i]) == LOW && (i + 1) != currentFloor) {
      currentFloor = i + 1;
      digitalWrite(MOTOR_UP_PIN, LOW);
      digitalWrite(MOTOR_DOWN_PIN, LOW);
      pendingCalls[currentFloor - 1] = false;
      openDoor();
      return;
    }
  }
}

void openDoor() {
  state = DOOR_OPEN;
  doorOpenFlag = true;
  digitalWrite(DOOR_LED_PIN, HIGH);
  doorOpenedAtMs = millis();
}

void handleDoorOpen() {
  if (millis() - doorOpenedAtMs >= DOOR_DWELL_MS) {
    doorOpenFlag = false;
    digitalWrite(DOOR_LED_PIN, LOW);
    state = IDLE;
  }
}

void onRequest() {
  uint8_t buf[3] = {currentFloor, state, (uint8_t)(doorOpenFlag ? 1 : 0)};
  Wire.write(buf, 3);
}

void onReceive(int numBytes) {
  if (numBytes < 1) return;
  requestedFloor = Wire.read();
}
