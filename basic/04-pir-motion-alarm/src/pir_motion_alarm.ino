/*
  PIR Motion Alarm
  -----------------
  Watches a PIR module's digital output and sounds a buzzer + lights an LED
  whenever motion is detected. Board: Arduino Uno.
*/

const uint8_t PIR_PIN = 2;
const uint8_t BUZZER_PIN = 7;
const uint8_t LED_PIN = 13;

bool lastState = LOW;

void setup() {
  pinMode(PIR_PIN, INPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(LED_PIN, OUTPUT);
  Serial.begin(9600);
  Serial.println("PIR warming up (30-60s)...");
  delay(3000); // brief settle time; full warm-up continues while sketch runs
}

void loop() {
  bool motion = digitalRead(PIR_PIN);

  if (motion && !lastState) {
    // Rising edge: motion just started.
    Serial.println("Motion detected!");
    tone(BUZZER_PIN, 2000); // continuous tone, non-blocking
  } else if (!motion && lastState) {
    // Falling edge: motion stopped (per the module's hold-time setting).
    Serial.println("Motion cleared.");
    noTone(BUZZER_PIN);
  }

  digitalWrite(LED_PIN, motion ? HIGH : LOW); // LED always mirrors raw PIR state
  lastState = motion;

  delay(50); // light debounce on the digital read
}
