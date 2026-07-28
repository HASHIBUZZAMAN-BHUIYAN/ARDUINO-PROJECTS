/*
  Sound-Activated Clap Switch
  -----------------------------
  Toggles a relay when it hears two claps in quick succession.
  Board: Arduino Nano.
*/

const uint8_t SOUND_PIN = 2;
const uint8_t RELAY_PIN = 4;

// Many cheap relay modules are active-LOW (LOW energizes the coil).
// Flip these two constants if your module works the opposite way.
const uint8_t RELAY_ON = LOW;
const uint8_t RELAY_OFF = HIGH;

const unsigned long CLAP_WINDOW_MS = 1000; // both claps must land inside this window
const unsigned long DEBOUNCE_MS = 100;     // ignore re-triggers within one "clap" event

bool relayState = false;
int clapCount = 0;
unsigned long firstClapTime = 0;
unsigned long lastPulseTime = 0;

void setup() {
  pinMode(SOUND_PIN, INPUT);
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, RELAY_OFF);
  Serial.begin(9600);
}

void loop() {
  if (digitalRead(SOUND_PIN) == HIGH) {
    unsigned long now = millis();

    if (now - lastPulseTime > DEBOUNCE_MS) { // treat this as a new, distinct clap
      lastPulseTime = now;

      if (clapCount == 0) {
        clapCount = 1;
        firstClapTime = now;
      } else if (now - firstClapTime <= CLAP_WINDOW_MS) {
        // Second clap arrived in time -- confirmed double-clap, toggle relay.
        toggleRelay();
        clapCount = 0;
      } else {
        // Too slow -- treat this pulse as a fresh "first clap" instead.
        clapCount = 1;
        firstClapTime = now;
      }
    }
  }

  // Reset a stale single clap that never got a follow-up.
  if (clapCount == 1 && millis() - firstClapTime > CLAP_WINDOW_MS) {
    clapCount = 0;
  }
}

void toggleRelay() {
  relayState = !relayState;
  digitalWrite(RELAY_PIN, relayState ? RELAY_ON : RELAY_OFF);
  Serial.println(relayState ? "Lamp ON" : "Lamp OFF");
}
