/*
  nRF24 Swarm Formation Robots - Follower
  ---------------------------------------------
  Tracks the leader's broadcast pose (best-effort) and runs closed-loop
  PID distance-holding using its own ultrasonic sensor. Set
  MY_FOLLOWER_ID and TARGET_DISTANCE_CM per physical unit.
  Board: Arduino Nano.
*/

#include <SPI.h>
#include <RF24.h>

const uint8_t MY_FOLLOWER_ID = 1; // 1 = Follower A, 2 = Follower B
const float TARGET_DISTANCE_CM = 40.0; // Follower A: 40, Follower B: 60

const uint8_t CE_PIN = 9, CSN_PIN = 10;
RF24 radio(CE_PIN, CSN_PIN);
const byte BROADCAST_PIPE[6] = "POSE1";
const byte STATUS_PIPE_A[6] = "STATA";
const byte STATUS_PIPE_B[6] = "STATB";

const uint8_t US_TRIG = 6, US_ECHO = 7;
const uint8_t MOTOR_PINS[4] = {2, 3, 4, 5};

struct Pose { int16_t x, y, heading; uint8_t seq; };
Pose lastPose = {0, 0, 0, 0};
uint8_t lastSeqSeen = 255;

struct FollowerStatus { uint8_t followerId; int16_t distErrorCm; uint16_t batteryMv; };

float integral = 0, lastError = 0;
unsigned long lastControlMs = 0;
const unsigned long CONTROL_INTERVAL_MS = 100;

unsigned long lastStatusMs = 0;
const unsigned long STATUS_INTERVAL_MS = 500;

void setup() {
  Serial.begin(9600);
  pinMode(US_TRIG, OUTPUT); pinMode(US_ECHO, INPUT);
  for (uint8_t i = 0; i < 4; i++) pinMode(MOTOR_PINS[i], OUTPUT);

  radio.begin();
  radio.setPALevel(RF24_PA_LOW);
  radio.openReadingPipe(1, BROADCAST_PIPE);
  radio.openWritingPipe(MY_FOLLOWER_ID == 1 ? STATUS_PIPE_A : STATUS_PIPE_B);
  radio.startListening();

  Serial.print("Follower "); Serial.print(MY_FOLLOWER_ID); Serial.println(" ready.");
}

void loop() {
  receivePoseIfAny();

  unsigned long now = millis();
  if (now - lastControlMs >= CONTROL_INTERVAL_MS) {
    lastControlMs = now;
    runDistanceHoldPID();
  }
  if (now - lastStatusMs >= STATUS_INTERVAL_MS) {
    lastStatusMs = now;
    sendStatus();
  }
}

void receivePoseIfAny() {
  uint8_t pipeNum;
  if (radio.available(&pipeNum)) {
    Pose incoming;
    radio.read(&incoming, sizeof(Pose));
    lastPose = incoming; // simply overwrite; a stale pose is fine by design
  }
}

long lastDistCm = 0;

void runDistanceHoldPID() {
  long distCm = readUltrasonicCm();
  if (distCm > 0 && distCm < 400) lastDistCm = distCm; // ignore obviously bad reads

  float error = lastDistCm - TARGET_DISTANCE_CM; // positive = too far, drive forward
  float dt = CONTROL_INTERVAL_MS / 1000.0;
  integral = constrain(integral + error * dt, -50, 50);
  float derivative = (error - lastError) / dt;
  lastError = error;

  float output = 3.0 * error + 0.1 * integral + 0.5 * derivative;
  driveForwardSpeed(constrain(output, -200, 200));
}

void driveForwardSpeed(float speed) {
  bool forward = speed > 0;
  uint8_t pwm = (uint8_t)constrain(abs(speed), 0, 255);
  digitalWrite(MOTOR_PINS[0], forward ? HIGH : LOW);
  digitalWrite(MOTOR_PINS[1], forward ? LOW : HIGH);
  digitalWrite(MOTOR_PINS[2], forward ? HIGH : LOW);
  digitalWrite(MOTOR_PINS[3], forward ? LOW : HIGH);
  analogWrite(MOTOR_PINS[0], pwm); // note: simplified - a real driver uses ENA/ENB for PWM
}

long readUltrasonicCm() {
  digitalWrite(US_TRIG, LOW); delayMicroseconds(2);
  digitalWrite(US_TRIG, HIGH); delayMicroseconds(10);
  digitalWrite(US_TRIG, LOW);
  long duration = pulseIn(US_ECHO, HIGH, 25000);
  return duration / 58;
}

void sendStatus() {
  FollowerStatus status;
  status.followerId = MY_FOLLOWER_ID;
  status.distErrorCm = (int16_t)(lastDistCm - TARGET_DISTANCE_CM);
  status.batteryMv = analogRead(A7) * (5000.0 / 1023.0); // simple resistor-divider battery sense

  radio.stopListening();
  radio.write(&status, sizeof(FollowerStatus));
  radio.startListening();
}
