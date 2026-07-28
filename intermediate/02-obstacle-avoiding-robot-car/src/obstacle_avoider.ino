/*
  Obstacle-Avoiding Robot Car
  ------------------------------
  Drives forward, and when it sees an obstacle, backs up, looks both ways
  with a servo-mounted ultrasonic sensor, then turns toward the clearer side.
  Board: Arduino Mega 2560.
*/

#include <Servo.h>

// Left motor
const uint8_t IN1 = 22, IN2 = 23, ENA = 5;
// Right motor
const uint8_t IN3 = 24, IN4 = 25, ENB = 6;

const uint8_t TRIG_PIN = 9;
const uint8_t ECHO_PIN = 10;
const uint8_t SERVO_PIN = 11;

const uint8_t DRIVE_SPEED = 180;   // 0-255 PWM
const float STOP_DISTANCE_CM = 20;

Servo panServo;

void setup() {
  pinMode(IN1, OUTPUT); pinMode(IN2, OUTPUT); pinMode(ENA, OUTPUT);
  pinMode(IN3, OUTPUT); pinMode(IN4, OUTPUT); pinMode(ENB, OUTPUT);
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  panServo.attach(SERVO_PIN);
  panServo.write(90); // sensor facing forward
  delay(300);

  Serial.begin(9600);
}

void loop() {
  float distance = readDistanceCm();

  if (distance > 0 && distance < STOP_DISTANCE_CM) {
    stopMotors();
    Serial.println("Obstacle detected - evasive maneuver");

    driveBackward();
    delay(400);
    stopMotors();
    delay(150);

    float leftClearance = lookDirection(150);  // servo left
    float rightClearance = lookDirection(30);  // servo right
    panServo.write(90);
    delay(200);

    if (leftClearance > rightClearance) {
      turnLeft();
    } else {
      turnRight();
    }
    delay(400);
    stopMotors();
    delay(100);
  } else {
    driveForward();
  }
}

// Sweeps the sensor to a given servo angle, waits for it to settle, and returns the distance seen there.
float lookDirection(uint8_t angle) {
  panServo.write(angle);
  delay(350); // let the servo physically get there before measuring
  return readDistanceCm();
}

float readDistanceCm() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  unsigned long duration = pulseIn(ECHO_PIN, HIGH, 30000UL);
  if (duration == 0) return 400; // no echo = treat as "clear"
  return duration * 0.0343 / 2.0;
}

void driveForward() {
  digitalWrite(IN1, HIGH); digitalWrite(IN2, LOW);
  digitalWrite(IN3, HIGH); digitalWrite(IN4, LOW);
  analogWrite(ENA, DRIVE_SPEED);
  analogWrite(ENB, DRIVE_SPEED);
}

void driveBackward() {
  digitalWrite(IN1, LOW); digitalWrite(IN2, HIGH);
  digitalWrite(IN3, LOW); digitalWrite(IN4, HIGH);
  analogWrite(ENA, DRIVE_SPEED);
  analogWrite(ENB, DRIVE_SPEED);
}

// In-place turns: one wheel forward, one wheel reverse.
void turnLeft() {
  digitalWrite(IN1, LOW); digitalWrite(IN2, HIGH);
  digitalWrite(IN3, HIGH); digitalWrite(IN4, LOW);
  analogWrite(ENA, DRIVE_SPEED);
  analogWrite(ENB, DRIVE_SPEED);
}

void turnRight() {
  digitalWrite(IN1, HIGH); digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW); digitalWrite(IN4, HIGH);
  analogWrite(ENA, DRIVE_SPEED);
  analogWrite(ENB, DRIVE_SPEED);
}

void stopMotors() {
  digitalWrite(IN1, LOW); digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW); digitalWrite(IN4, LOW);
  analogWrite(ENA, 0);
  analogWrite(ENB, 0);
}
