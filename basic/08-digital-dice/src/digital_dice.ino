/*
  Digital Dice
  -------------
  Button press "rolls" a virtual die; result is shown on 7 LEDs arranged
  in the classic dot pattern. Board: Arduino Uno Q (3.3V logic).
*/

const uint8_t LED_PINS[7] = {2, 3, 4, 5, 6, 7, 8};
// LED index layout:  0 1
//                      3
//                    2 4
//                    5 6
const uint8_t BUTTON_PIN = 12;
const uint8_t SEED_PIN = A5; // left unconnected/floating, used only for random noise

// Which of the 7 LEDs light up for each face value (index 0 = face "1").
const bool DICE_PATTERNS[6][7] = {
  {0,0,0,1,0,0,0}, // 1: center only
  {1,0,0,0,0,0,1}, // 2: two corners
  {1,0,0,1,0,0,1}, // 3: two corners + center
  {1,1,0,0,0,1,1}, // 4: four corners only
  {1,1,0,1,0,1,1}, // 5: four corners + center
  {1,1,1,0,1,1,1}, // 6: four corners + both middle-row sides
};

bool lastButtonState = LOW;
unsigned long lastDebounceTime = 0;
const unsigned long DEBOUNCE_MS = 40;

void setup() {
  for (uint8_t i = 0; i < 7; i++) pinMode(LED_PINS[i], OUTPUT);
  pinMode(BUTTON_PIN, INPUT);
  randomSeed(analogRead(SEED_PIN)); // floating pin picks up electrical noise as an entropy source
  Serial.begin(9600);
}

void loop() {
  bool reading = digitalRead(BUTTON_PIN);

  if (reading != lastButtonState) {
    lastDebounceTime = millis();
  }

  if ((millis() - lastDebounceTime) > DEBOUNCE_MS) {
    if (reading == HIGH && lastButtonState == LOW) {
      rollDice();
    }
  }

  lastButtonState = reading;
}

void rollDice() {
  // Quick "rolling" animation: flash a few random faces before settling.
  for (uint8_t i = 0; i < 8; i++) {
    showFace(random(1, 7));
    delay(60);
  }

  int result = random(1, 7); // random(1,7) -> 1..6 inclusive
  showFace(result);
  Serial.print("Rolled: ");
  Serial.println(result);
}

void showFace(int face) {
  const bool *pattern = DICE_PATTERNS[face - 1];
  for (uint8_t i = 0; i < 7; i++) {
    digitalWrite(LED_PINS[i], pattern[i] ? HIGH : LOW);
  }
}
