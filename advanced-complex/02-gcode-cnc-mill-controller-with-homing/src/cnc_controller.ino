/*
  G-Code CNC Mill Controller with Closed-Loop Homing
  ---------------------------------------------------
  Parses a small G-code subset from Serial or SD, drives 3 axes with
  AccelStepper, and performs interrupt-driven limit-switch homing.
  Board: Arduino Mega 2560.
*/

#include <AccelStepper.h>
#include <SPI.h>
#include <SD.h>

const uint8_t STEP_PINS[3] = {22, 24, 26};
const uint8_t DIR_PINS[3]  = {23, 25, 27};
const uint8_t LIMIT_PINS[3] = {2, 3, 18};
const uint8_t SPINDLE_PIN = 28;
const uint8_t SD_CS = 53;

const float STEPS_PER_MM = 80.0;
const float HOMING_SPEED = 400;     // steps/s, fast approach
const float HOMING_BACKOFF_MM = 5.0;
const float HOMING_SLOW_SPEED = 100; // steps/s, precise re-approach

AccelStepper stepper[3] = {
  AccelStepper(AccelStepper::DRIVER, STEP_PINS[0], DIR_PINS[0]),
  AccelStepper(AccelStepper::DRIVER, STEP_PINS[1], DIR_PINS[1]),
  AccelStepper(AccelStepper::DRIVER, STEP_PINS[2], DIR_PINS[2])
};

volatile bool limitHit[3] = {false, false, false};

struct Move {
  bool valid = false;
  bool isHome = false;
  bool spindleOn = false, spindleCmd = false;
  float x, y, z, feed;
};

const uint8_t QUEUE_SIZE = 8;
Move queue[QUEUE_SIZE];
uint8_t qHead = 0, qTail = 0, qCount = 0;

bool runningFromSD = false;
File sdFile;

void limitISR0() { limitHit[0] = true; }
void limitISR1() { limitHit[1] = true; }
void limitISR2() { limitHit[2] = true; }

void setup() {
  Serial.begin(115200);

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

  if (!SD.begin(SD_CS)) Serial.println("SD init failed (serial mode only)");

  Serial.println("CNC controller ready. Send G-code lines, or RUN to stream PROGRAM.GCO from SD.");
}

void loop() {
  runQueuedMotion();
  fillQueueFromSerial();
  if (runningFromSD) fillQueueFromSD();
  dispatchNextIfIdle();
}

bool allAxesIdle() {
  return stepper[0].distanceToGo() == 0 &&
         stepper[1].distanceToGo() == 0 &&
         stepper[2].distanceToGo() == 0;
}

void runQueuedMotion() {
  for (uint8_t i = 0; i < 3; i++) stepper[i].run();
}

void dispatchNextIfIdle() {
  static bool dispatched = false;
  if (!allAxesIdle()) return;
  if (dispatched) { Serial.println("ok"); dispatched = false; }
  if (qCount == 0) return;

  Move m = queue[qHead];
  qHead = (qHead + 1) % QUEUE_SIZE;
  qCount--;

  if (m.isHome) {
    homeAllAxes();
    dispatched = true;
    return;
  }
  if (m.spindleCmd) {
    digitalWrite(SPINDLE_PIN, m.spindleOn ? HIGH : LOW);
  }
  stepper[0].moveTo(m.x * STEPS_PER_MM);
  stepper[1].moveTo(m.y * STEPS_PER_MM);
  stepper[2].moveTo(m.z * STEPS_PER_MM);
  float feedSteps = m.feed > 0 ? m.feed : 800;
  for (uint8_t i = 0; i < 3; i++) stepper[i].setMaxSpeed(feedSteps);
  dispatched = true;
}

void homeAllAxes() {
  for (uint8_t axis = 0; axis < 3; axis++) {
    limitHit[axis] = false;
    stepper[axis].setMaxSpeed(HOMING_SPEED);
    stepper[axis].move(-100000); // fast approach toward switch
    while (!limitHit[axis]) stepper[axis].run();
    stepper[axis].stop();
    stepper[axis].setCurrentPosition(0);

    // back off
    stepper[axis].move(HOMING_BACKOFF_MM * STEPS_PER_MM);
    while (stepper[axis].distanceToGo() != 0) stepper[axis].run();

    // slow re-approach for a repeatable zero
    limitHit[axis] = false;
    stepper[axis].setMaxSpeed(HOMING_SLOW_SPEED);
    stepper[axis].move(-(HOMING_BACKOFF_MM * 2) * STEPS_PER_MM);
    while (!limitHit[axis]) stepper[axis].run();
    stepper[axis].stop();
    stepper[axis].setCurrentPosition(0);
  }
  Serial.println("Homing complete.");
}

void enqueue(const Move &m) {
  if (qCount >= QUEUE_SIZE) return; // caller should not send faster than "ok" pace
  queue[qTail] = m;
  qTail = (qTail + 1) % QUEUE_SIZE;
  qCount++;
}

void fillQueueFromSerial() {
  if (!Serial.available() || qCount >= QUEUE_SIZE) return;
  String line = Serial.readStringUntil('\n');
  line.trim();
  if (line.length() == 0) return;

  if (line.equalsIgnoreCase("RUN")) {
    sdFile = SD.open("PROGRAM.GCO");
    runningFromSD = sdFile ? true : false;
    Serial.println(runningFromSD ? "Streaming PROGRAM.GCO from SD" : "PROGRAM.GCO not found");
    return;
  }
  parseAndEnqueue(line);
}

void fillQueueFromSD() {
  if (qCount >= QUEUE_SIZE) return;
  if (!sdFile.available()) {
    sdFile.close();
    runningFromSD = false;
    Serial.println("SD program complete.");
    return;
  }
  String line = sdFile.readStringUntil('\n');
  line.trim();
  if (line.length() > 0) parseAndEnqueue(line);
}

void parseAndEnqueue(const String &line) {
  Move m;
  m.valid = true;
  m.x = stepper[0].currentPosition() / STEPS_PER_MM;
  m.y = stepper[1].currentPosition() / STEPS_PER_MM;
  m.z = stepper[2].currentPosition() / STEPS_PER_MM;
  m.feed = 0;

  if (line.startsWith("G28")) {
    m.isHome = true;
    enqueue(m);
    return;
  }
  if (line.startsWith("M3")) { m.spindleCmd = true; m.spindleOn = true; enqueue(m); return; }
  if (line.startsWith("M5")) { m.spindleCmd = true; m.spindleOn = false; enqueue(m); return; }

  if (line.startsWith("G0") || line.startsWith("G1")) {
    int xi = line.indexOf('X');
    int yi = line.indexOf('Y');
    int zi = line.indexOf('Z');
    int fi = line.indexOf('F');
    if (xi >= 0) m.x = line.substring(xi + 1).toFloat();
    if (yi >= 0) m.y = line.substring(yi + 1).toFloat();
    if (zi >= 0) m.z = line.substring(zi + 1).toFloat();
    if (fi >= 0) m.feed = line.substring(fi + 1).toFloat();
    enqueue(m);
  }
}
