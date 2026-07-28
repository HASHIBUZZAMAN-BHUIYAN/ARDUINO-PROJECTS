/*
  Morse Code LED Beacon
  ----------------------
  Repeats a fixed message on a single LED using standard Morse timing ratios.
  Board: Arduino Uno (works on any AVR board with a digital output pin).
*/

const uint8_t LED_PIN = 8;

// Base unit length in milliseconds. All other timings are ratios of this,
// which is the whole point of Morse code timing -- change this one value
// to speed up or slow down the entire beacon.
const uint16_t DOT_MS = 150;

const char *MESSAGE = "SOS EDGECOST";

// Morse lookup table for A-Z. Index 0 = 'A'. '.' = dot, '-' = dash.
const char *MORSE_TABLE[26] = {
  ".-", "-...", "-.-.", "-..", ".",   "..-.", "--.",  "....", "..",
  ".---", "-.-", ".-..", "--", "-.",  "---",  ".--.", "--.-", ".-.",
  "...", "-",   "..-",  "...-", ".--", "-..-", "-.--", "--.."
};

void setup() {
  pinMode(LED_PIN, OUTPUT);
  Serial.begin(9600);
}

void loop() {
  Serial.print("Transmitting: ");
  Serial.println(MESSAGE);

  for (const char *p = MESSAGE; *p != '\0'; p++) {
    char c = *p;

    if (c == ' ') {
      // Word gap: 7 units total. We already emitted a 3-unit letter gap
      // after the previous letter, so only 4 more units are needed here.
      delay(DOT_MS * 4);
      continue;
    }

    if (c >= 'a' && c <= 'z') c -= 32; // normalize to uppercase
    if (c < 'A' || c > 'Z') continue;   // skip anything we can't encode

    flashPattern(MORSE_TABLE[c - 'A']);
    delay(DOT_MS * 2); // finish the 3-unit inter-letter gap (1 unit already sent after last symbol)
  }

  delay(DOT_MS * 7); // pause before repeating the whole message
}

// Flashes one letter's dot/dash pattern, leaving a 1-unit gap after each symbol.
void flashPattern(const char *pattern) {
  for (const char *s = pattern; *s != '\0'; s++) {
    digitalWrite(LED_PIN, HIGH);
    delay(*s == '.' ? DOT_MS : DOT_MS * 3);
    digitalWrite(LED_PIN, LOW);
    delay(DOT_MS); // inter-symbol gap within the same letter
  }
}
