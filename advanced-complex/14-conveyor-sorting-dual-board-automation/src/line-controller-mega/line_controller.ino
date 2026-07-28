/*
  Conveyor Sorting - Line Controller (I2C master)
  -------------------------------------------------------
  Closed-loop PID conveyor belt speed control using an IR break-beam
  for feedback, object counting, and I2C coordination with the sorter
  node to divert items by detected color. Board: Arduino Mega 2560.
*/

#include <Wire.h>

const uint8_t SORTER_ADDR = 0x10;
const uint8_t MOTOR_IN1 = 8, MOTOR_IN2 = 9, MOTOR_ENA = 10;
const uint8_t BEAM_PIN = 2;

volatile unsigned long beamPulseCount = 0;
volatile unsigned long lastPulseMs = 0;

float targetPulseHz = 4.0; // calibrate to your desired belt speed
float kp = 20.0, ki = 2.0, kd = 1.0;
float integral = 0, lastError = 0;
int pwmOutput = 150;

unsigned long lastControlMs = 0;
const unsigned long CONTROL_INTERVAL_MS = 500;

unsigned long itemCount = 0;
unsigned long lastQueryMs = 0;
const unsigned long DIVERT_DELAY_MS = 1500; // time for item to travel from beam to diverter

void beamISR() {
  beamPulseCount++;
  lastPulseMs = millis();
}

void setup() {
  Serial.begin(9600);
  Wire.begin(); // master

  pinMode(MOTOR_IN1, OUTPUT); pinMode(MOTOR_IN2, OUTPUT); pinMode(MOTOR_ENA, OUTPUT);
  digitalWrite(MOTOR_IN1, HIGH); digitalWrite(MOTOR_IN2, LOW); // forward direction

  pinMode(BEAM_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(BEAM_PIN), beamISR, FALLING);

  Serial.println("Line controller ready.");
}

void loop() {
  unsigned long now = millis();

  if (now - lastControlMs >= CONTROL_INTERVAL_MS) {
    lastControlMs = now;
    runSpeedPID();
  }

  static unsigned long lastCount = 0;
  noInterrupts();
  unsigned long count = beamPulseCount;
  interrupts();
  if (count != lastCount) {
    lastCount = count;
    itemCount++;
    scheduleDivert();
  }

  serviceScheduledDiverts();
}

void runSpeedPID() {
  noInterrupts();
  unsigned long count = beamPulseCount;
  beamPulseCount = 0;
  interrupts();

  float measuredHz = count / (CONTROL_INTERVAL_MS / 1000.0);
  float error = targetPulseHz - measuredHz;
  integral += error * (CONTROL_INTERVAL_MS / 1000.0);
  integral = constrain(integral, -50, 50);
  float derivative = (error - lastError) / (CONTROL_INTERVAL_MS / 1000.0);
  lastError = error;

  float output = kp * error + ki * integral + kd * derivative;
  pwmOutput = constrain(pwmOutput + (int)output, 0, 255);
  analogWrite(MOTOR_ENA, pwmOutput);

  Serial.print("Belt speed Hz="); Serial.print(measuredHz);
  Serial.print(" PWM="); Serial.println(pwmOutput);
}

struct PendingDivert { unsigned long dueMs; bool active; };
PendingDivert pending = {0, false};

void scheduleDivert() {
  pending.dueMs = millis() + DIVERT_DELAY_MS;
  pending.active = true;
}

void serviceScheduledDiverts() {
  if (!pending.active) return;
  if (millis() < pending.dueMs) return;
  pending.active = false;

  Wire.requestFrom(SORTER_ADDR, (uint8_t)1);
  uint8_t color = 0;
  if (Wire.available()) color = Wire.read();

  uint8_t lane = colorToLane(color);
  Wire.beginTransmission(SORTER_ADDR);
  Wire.write(lane);
  Wire.endTransmission();

  Serial.print("Item #"); Serial.print(itemCount);
  Serial.print(" color="); Serial.print(color);
  Serial.print(" -> lane="); Serial.println(lane);
}

uint8_t colorToLane(uint8_t color) {
  if (color == 1) return 1; // red -> left
  if (color == 3) return 2; // blue -> right
  return 0;                  // green/unknown -> straight
}
