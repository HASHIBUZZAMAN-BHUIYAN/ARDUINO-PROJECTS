/*
  nRF24 Swarm Formation Robots - Leader
  ------------------------------------------
  Dead-reckons its own pose from wheel encoders and broadcasts it
  best-effort over nRF24L01 for followers to track.
  Board: Arduino Uno.
*/

#include <SPI.h>
#include <RF24.h>

const uint8_t CE_PIN = 9, CSN_PIN = 10;
RF24 radio(CE_PIN, CSN_PIN);
const byte BROADCAST_PIPE[6] = "POSE1";
const byte STATUS_PIPE_A[6] = "STATA";
const byte STATUS_PIPE_B[6] = "STATB";

const uint8_t ENC_L_A = 2, ENC_L_B = A0;
const uint8_t ENC_R_A = 3, ENC_R_B = A1;
const uint8_t MOTOR_PINS[4] = {4, 5, 6, 7};

volatile long ticksL = 0, ticksR = 0;
const float WHEEL_BASE_CM = 15.0;
const float TICKS_PER_CM = 20.0;

struct Pose { int16_t x, y, heading; uint8_t seq; };
Pose pose = {0, 0, 0, 0};

struct FollowerStatus { uint8_t followerId; int16_t distErrorCm; uint16_t batteryMv; };

unsigned long lastBroadcastMs = 0;
const unsigned long BROADCAST_INTERVAL_MS = 100;

void encLA_ISR() { digitalRead(ENC_L_B) ? ticksL++ : ticksL--; }
void encRA_ISR() { digitalRead(ENC_R_B) ? ticksR++ : ticksR--; }

void setup() {
  Serial.begin(9600);
  pinMode(ENC_L_A, INPUT_PULLUP); pinMode(ENC_L_B, INPUT_PULLUP);
  pinMode(ENC_R_A, INPUT_PULLUP); pinMode(ENC_R_B, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(ENC_L_A), encLA_ISR, RISING);
  attachInterrupt(digitalPinToInterrupt(ENC_R_A), encRA_ISR, RISING);

  for (uint8_t i = 0; i < 4; i++) pinMode(MOTOR_PINS[i], OUTPUT);

  radio.begin();
  radio.setPALevel(RF24_PA_LOW);
  radio.openWritingPipe(BROADCAST_PIPE);
  radio.openReadingPipe(1, STATUS_PIPE_A);
  radio.openReadingPipe(2, STATUS_PIPE_B);
  radio.stopListening();

  driveForwardScript();
  Serial.println("Leader ready, broadcasting pose.");
}

void loop() {
  updatePose();
  receiveFollowerStatus();

  unsigned long now = millis();
  if (now - lastBroadcastMs >= BROADCAST_INTERVAL_MS) {
    lastBroadcastMs = now;
    broadcastPose();
  }
}

void driveForwardScript() {
  // simple preset script: drive forward slowly for the demo
  digitalWrite(MOTOR_PINS[0], HIGH); digitalWrite(MOTOR_PINS[1], LOW);
  digitalWrite(MOTOR_PINS[2], HIGH); digitalWrite(MOTOR_PINS[3], LOW);
}

void updatePose() {
  noInterrupts();
  long l = ticksL, r = ticksR;
  interrupts();

  float distCm = ((l + r) / 2.0) / TICKS_PER_CM;
  float headingRad = ((r - l) / TICKS_PER_CM) / WHEEL_BASE_CM;

  pose.heading = (int16_t)(headingRad * 180.0 / PI);
  pose.x = (int16_t)(distCm * cos(headingRad));
  pose.y = (int16_t)(distCm * sin(headingRad));
}

void broadcastPose() {
  pose.seq++;
  radio.stopListening();
  radio.write(&pose, sizeof(Pose)); // best-effort; no retry logic needed by design
  radio.startListening();
}

void receiveFollowerStatus() {
  uint8_t pipeNum;
  if (radio.available(&pipeNum)) {
    FollowerStatus status;
    radio.read(&status, sizeof(FollowerStatus));
    Serial.print("Follower "); Serial.print(status.followerId);
    Serial.print(" distErr="); Serial.print(status.distErrorCm);
    Serial.print("cm batt="); Serial.print(status.batteryMv); Serial.println("mV");
  }
}
