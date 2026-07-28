/*
  Light-Activated Night Light
  -----------------------------
  Reads an LDR voltage divider and switches an LED on at dusk, off at dawn,
  with hysteresis between the two thresholds to prevent flicker.
  Board: Arduino Nano.
*/

const uint8_t LDR_PIN = A0;
const uint8_t LED_PIN = 9; // PWM pin for fade-in

// Two separate thresholds create hysteresis: once dark, it must get
// noticeably brighter than the trigger point before switching back off.
const int DARK_THRESHOLD = 350;  // below this reading -> considered "dark"
const int LIGHT_THRESHOLD = 500; // above this reading -> considered "light"

bool lampOn = false;

void setup() {
  pinMode(LED_PIN, OUTPUT);
  Serial.begin(9600);
}

void loop() {
  int level = analogRead(LDR_PIN);
  Serial.print("light level: ");
  Serial.println(level);

  if (!lampOn && level < DARK_THRESHOLD) {
    lampOn = true;
    fadeIn();
  } else if (lampOn && level > LIGHT_THRESHOLD) {
    lampOn = false;
    analogWrite(LED_PIN, 0);
  }
  // Between the two thresholds: intentionally do nothing, hold current state.

  delay(500); // ambient light changes slowly; no need to poll faster
}

// Ramps brightness up gradually instead of snapping straight to full on.
void fadeIn() {
  for (int duty = 0; duty <= 255; duty += 5) {
    analogWrite(LED_PIN, duty);
    delay(15);
  }
}
