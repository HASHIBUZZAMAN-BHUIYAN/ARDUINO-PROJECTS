/*
  Servo Pan-Tilt Camera Mount
  ------------------------------
  Joystick deflection drives pan/tilt servo speed (not direct position) for
  smooth, analog-feeling control. Button press re-centers both axes.
  Board: Arduino Nano.
*/

#include <Servo.h>

const uint8_t JOY_X_PIN = A0;
const uint8_t JOY_Y_PIN = A1;
const uint8_t JOY_BTN_PIN = 8;
const uint8_t PAN_SERVO_PIN = 9;
const uint8_t TILT_SERVO_PIN = 10;

const int JOY_CENTER = 512;
const int DEADZONE = 60;       // ignore small drift around center
const float MAX_SPEED = 1.2;   // degrees per loop iteration at full deflection
const uint8_t MIN_ANGLE = 10;
const uint8_t MAX_ANGLE = 170;

Servo panServo, tiltServo;
float panAngle = 90;
float tiltAngle = 90;
bool lastButtonState = HIGH;

void setup() {
  panServo.attach(PAN_SERVO_PIN);
  tiltServo.attach(TILT_SERVO_PIN);
  pinMode(JOY_BTN_PIN, INPUT_PULLUP);

  panServo.write(panAngle);
  tiltServo.write(tiltAngle);
  Serial.begin(9600);
}

void loop() {
  panAngle = updateAxis(analogRead(JOY_X_PIN), panAngle);
  tiltAngle = updateAxis(analogRead(JOY_Y_PIN), tiltAngle);

  panServo.write((int)panAngle);
  tiltServo.write((int)tiltAngle);

  bool buttonState = digitalRead(JOY_BTN_PIN);
  if (buttonState == LOW && lastButtonState == HIGH) {
    delay(20); // debounce
    panAngle = 90;
    tiltAngle = 90;
    Serial.println("Re-centered.");
  }
  lastButtonState = buttonState;

  delay(15); // ~66Hz update rate keeps motion smooth without overloading servos
}

// Converts a raw joystick reading into a speed proportional to how far the
// stick is pushed from center, then integrates that speed into the current angle.
float updateAxis(int raw, float currentAngle) {
  int offset = raw - JOY_CENTER;

  if (abs(offset) < DEADZONE) return currentAngle; // stick is effectively centered

  float normalized = constrain((float)offset / JOY_CENTER, -1.0, 1.0);
  float speed = normalized * MAX_SPEED;

  float newAngle = currentAngle + speed;
  return constrain(newAngle, MIN_ANGLE, MAX_ANGLE);
}
