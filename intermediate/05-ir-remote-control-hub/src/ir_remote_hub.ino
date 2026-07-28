/*
  IR Remote Control Hub
  ------------------------
  Decodes a generic IR remote and maps its buttons to 4 relay channels,
  plus "all on"/"all off" convenience buttons. Board: Arduino Uno Q.

  Run once with LEARNING_MODE = true to discover your remote's codes via
  Serial Monitor, then fill in the BUTTON_* constants and set it false.
*/

#include <IRremote.hpp>

const uint8_t IR_RECEIVE_PIN = 2;
const uint8_t RELAY_PINS[4] = {4, 5, 6, 7};

bool LEARNING_MODE = true;

// Replace these placeholders with the actual codes printed by your remote
// while LEARNING_MODE is true.
const uint32_t BUTTON_CH_A     = 0xE0E020DF;
const uint32_t BUTTON_CH_B     = 0xE0E0A05F;
const uint32_t BUTTON_CH_C     = 0xE0E0609F;
const uint32_t BUTTON_CH_D     = 0xE0E010EF;
const uint32_t BUTTON_ALL_ON   = 0xE0E0E01F;
const uint32_t BUTTON_ALL_OFF  = 0xE0E0906F;

bool relayState[4] = {false, false, false, false};

void setup() {
  Serial.begin(9600);
  for (uint8_t i = 0; i < 4; i++) {
    pinMode(RELAY_PINS[i], OUTPUT);
    digitalWrite(RELAY_PINS[i], LOW);
  }

  IrReceiver.begin(IR_RECEIVE_PIN, ENABLE_LED_FEEDBACK);
  Serial.println(LEARNING_MODE
    ? "LEARNING MODE: press remote buttons to print their codes."
    : "Ready. Waiting for remote commands.");
}

void loop() {
  if (!IrReceiver.decode()) return;

  uint32_t code = IrReceiver.decodedIRData.decodedRawData;

  if (LEARNING_MODE) {
    Serial.print("Button code: 0x");
    Serial.println(code, HEX);
  } else {
    handleCommand(code);
  }

  IrReceiver.resume(); // ready for the next button press
}

void handleCommand(uint32_t code) {
  if (code == BUTTON_CH_A) toggleRelay(0);
  else if (code == BUTTON_CH_B) toggleRelay(1);
  else if (code == BUTTON_CH_C) toggleRelay(2);
  else if (code == BUTTON_CH_D) toggleRelay(3);
  else if (code == BUTTON_ALL_ON) setAll(true);
  else if (code == BUTTON_ALL_OFF) setAll(false);
  else Serial.println("Unrecognized button, ignoring.");
}

void toggleRelay(uint8_t index) {
  relayState[index] = !relayState[index];
  digitalWrite(RELAY_PINS[index], relayState[index] ? HIGH : LOW);
  Serial.print("Channel ");
  Serial.print((char)('A' + index));
  Serial.println(relayState[index] ? " ON" : " OFF");
}

void setAll(bool on) {
  for (uint8_t i = 0; i < 4; i++) {
    relayState[i] = on;
    digitalWrite(RELAY_PINS[i], on ? HIGH : LOW);
  }
  Serial.println(on ? "All channels ON" : "All channels OFF");
}
