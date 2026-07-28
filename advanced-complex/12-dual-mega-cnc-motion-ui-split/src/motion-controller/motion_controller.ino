/*
  Dual-Board CNC - Motion Controller
  --------------------------------------
  Owns all real-time motion: 3-axis stepping, closed-loop limit-switch
  homing, and the spindle relay. Receives framed binary motion packets
  from the UI controller over Serial2 and only ACKs once idle.
  Board: Arduino Mega 2560.
*/

#include <AccelStepper.h>

const uint8_t STEP_PINS[3] = {22, 24, 26};
const uint8_t DIR_PINS[3]  = {23, 25, 27};
const uint8_t LIMIT_PINS[3] = {2, 3, 18};
const uint8_t SPINDLE_PIN = 28;

const float STEPS_PER_MM = 80.0;
const float HOMING_SPEED = 400, HOMING_SLOW_SPEED = 100;
const float HOMING_BACKOFF_MM = 5.0;

AccelStepper stepper[3] = {
  AccelStepper(AccelStepper::DRIVER, STEP_PINS[0], DIR_PINS[0]),
  AccelStepper(AccelStepper::DRIVER, STEP_PINS[1], DIR_PINS[1]),
  AccelStepper(AccelStepper::DRIVER, STEP_PINS[2], DIR_PINS[2])
};

volatile bool limitHit[3] = {false, false, false};

#pragma pack(push, 1)
struct MovePacket {
  uint8_t start; // 0x7E
  uint8_t cmdId;
  int16_t x, y, z;
  uint16_t feedRate;
  uint8_t crc8;
  uint8_t end; // 0x7F
};
#pragma pack(pop)

unsigned long lastStatusMs = 0;

void limitISR0() { limitHit[0] = true; }
void limitISR1() { limitHit[1] = true; }
void limitISR2() { limitHit[2] = true; }

void setup() {
  Serial.begin(115200);   // USB debug console
  Serial2.begin(115200);  // link to UI controller

  for (uint8_t i = 0; i < 3; i++) {
    stepper[i].setMaxSpeed(2000);
    stepper[i].setAcceleration(800);
    pinMode(LIMIT_PINS[i], INPUT_PULLUP);
  }
  attachInterrupt(digitalPinToInterrupt(LIMIT_PINS[0]), limitISR0, FALLING);
  attachInterrupt(digitalPinToInterrupt(LIMIT_PINS[1]), limitISR1, FALLING);
  attachInterrupt(digitalPinToInterrupt(LIMIT_PINS[2]), limitISR2, FALLING);

  pinMode(SPINDLE_PIN, OUTPUT);
  digitalWrite(SPINDLE_PIN, LOW);

  Serial.println("Motion controller ready.");
}

void loop() {
  for (uint8_t i = 0; i < 3; i++) stepper[i].run();

  readPacketIfAny();
  streamStatusIfDue();
}

bool allAxesIdle() {
  return stepper[0].distanceToGo() == 0 &&
         stepper[1].distanceToGo() == 0 &&
         stepper[2].distanceToGo() == 0;
}

void readPacketIfAny() {
  if (Serial2.available() < (int)sizeof(MovePacket)) return;

  MovePacket pkt;
  Serial2.readBytes((char *)&pkt, sizeof(MovePacket));

  if (pkt.start != 0x7E || pkt.end != 0x7F) return; // malformed, drop

  uint8_t computed = crc8((uint8_t *)&pkt, sizeof(MovePacket) - 2);
  if (computed != pkt.crc8) {
    Serial.println("CRC8 mismatch, dropping packet.");
    return;
  }

  if (!allAxesIdle()) {
    Serial.println("Busy - unexpected packet ignored (UI controller should wait for ACK).");
    return;
  }

  if (pkt.cmdId == 0) {
    homeAllAxes();
  } else {
    stepper[0].moveTo(pkt.x * STEPS_PER_MM / 10.0);
    stepper[1].moveTo(pkt.y * STEPS_PER_MM / 10.0);
    stepper[2].moveTo(pkt.z * STEPS_PER_MM / 10.0);
    float feed = pkt.feedRate > 0 ? pkt.feedRate : 800;
    for (uint8_t i = 0; i < 3; i++) stepper[i].setMaxSpeed(feed);
  }

  waitForMotionThenAck(pkt.cmdId);
}

void waitForMotionThenAck(uint8_t cmdId) {
  while (!allAxesIdle()) {
    for (uint8_t i = 0; i < 3; i++) stepper[i].run();
  }
  Serial2.print("ACK,"); Serial2.println(cmdId);
}

void homeAllAxes() {
  for (uint8_t axis = 0; axis < 3; axis++) {
    limitHit[axis] = false;
    stepper[axis].setMaxSpeed(HOMING_SPEED);
    stepper[axis].move(-100000);
    while (!limitHit[axis]) stepper[axis].run();
    stepper[axis].stop();
    stepper[axis].setCurrentPosition(0);

    stepper[axis].move(HOMING_BACKOFF_MM * STEPS_PER_MM);
    while (stepper[axis].distanceToGo() != 0) stepper[axis].run();

    limitHit[axis] = false;
    stepper[axis].setMaxSpeed(HOMING_SLOW_SPEED);
    stepper[axis].move(-(HOMING_BACKOFF_MM * 2) * STEPS_PER_MM);
    while (!limitHit[axis]) stepper[axis].run();
    stepper[axis].stop();
    stepper[axis].setCurrentPosition(0);
  }
}

void streamStatusIfDue() {
  unsigned long now = millis();
  if (now - lastStatusMs < 250) return;
  lastStatusMs = now;

  Serial2.print("POS,");
  Serial2.print(stepper[0].currentPosition() / STEPS_PER_MM); Serial2.print(",");
  Serial2.print(stepper[1].currentPosition() / STEPS_PER_MM); Serial2.print(",");
  Serial2.print(stepper[2].currentPosition() / STEPS_PER_MM); Serial2.print(",");
  Serial2.println(allAxesIdle() ? "IDLE" : "MOVING");
}

uint8_t crc8(const uint8_t *data, size_t len) {
  uint8_t crc = 0;
  for (size_t i = 0; i < len; i++) {
    crc ^= data[i];
    for (uint8_t b = 0; b < 8; b++) {
      crc = (crc & 0x80) ? (crc << 1) ^ 0x07 : (crc << 1);
    }
  }
  return crc;
}
