/*
  Analog Servo Gauge
  --------------------
  Reads a potentiometer and drives a servo needle to match its position.
  Board: Arduino Nano.
*/

#include <Servo.h>

const uint8_t POT_PIN = A0;
const uint8_t SERVO_PIN = 9;

// Trimmed slightly inside the full 0-180 range to avoid straining
// the servo's mechanical end stops.
const int MIN_ANGLE = 10;
const int MAX_ANGLE = 170;

Servo needle;
int lastAngle = -1;

void setup() {
  needle.attach(SERVO_PIN);
  Serial.begin(9600);
}

void loop() {
  int raw = analogRead(POT_PIN); // 0-1023
  int angle = map(raw, 0, 1023, MIN_ANGLE, MAX_ANGLE);

  // Deadband: only move the servo if the target changed meaningfully.
  // Without this, ADC noise on a stationary knob makes the servo buzz.
  if (abs(angle - lastAngle) > 1) {
    needle.write(angle);
    lastAngle = angle;
    Serial.print("angle: ");
    Serial.println(angle);
  }

  delay(20);
}
