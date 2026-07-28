/*
  Ultrasonic Parking Sensor
  --------------------------
  Beeps faster as an obstacle gets closer; solid tone inside the danger zone.
  Board: Arduino Uno.
*/

const uint8_t TRIG_PIN = 9;
const uint8_t ECHO_PIN = 10;
const uint8_t BUZZER_PIN = 6;

const float DANGER_CM = 10.0;   // continuous buzz below this distance
const float MAX_RANGE_CM = 200; // ignore readings beyond this (out of useful range)

void setup() {
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  Serial.begin(9600);
}

void loop() {
  float distance = readDistanceCm();

  if (distance <= 0 || distance > MAX_RANGE_CM) {
    // No valid echo (out of range or nothing reflecting) -- stay quiet.
    noTone(BUZZER_PIN);
    Serial.println("out of range");
    delay(200);
    return;
  }

  Serial.print("distance: ");
  Serial.print(distance);
  Serial.println(" cm");

  if (distance <= DANGER_CM) {
    // Too close: solid tone, no beep gaps.
    tone(BUZZER_PIN, 1000);
    delay(50);
    return;
  }

  // Map distance to a beep gap: closer object = shorter gap = faster beeping.
  // 10 cm -> ~40ms gap, 200 cm -> ~600ms gap.
  int gapMs = map((int)distance, DANGER_CM, MAX_RANGE_CM, 40, 600);
  tone(BUZZER_PIN, 1000, 40); // short beep
  delay(gapMs);
}

// Fires the trigger pulse and converts the echo time into centimeters.
float readDistanceCm() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10); // HC-SR04 datasheet requires a >=10us trigger pulse
  digitalWrite(TRIG_PIN, LOW);

  // 30ms timeout keeps loop() responsive even if no echo ever returns.
  unsigned long duration = pulseIn(ECHO_PIN, HIGH, 30000UL);
  if (duration == 0) return -1;

  // Speed of sound ~343 m/s = 0.0343 cm/us; divide by 2 for the round trip.
  return duration * 0.0343 / 2.0;
}
