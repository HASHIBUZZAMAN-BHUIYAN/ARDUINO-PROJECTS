/*
  I2C Multi-Drop Network - Relay Bank Node (0x08)
  ----------------------------------------------------
  I2C slave exposing a 4-relay bitmask: readable via requestFrom(),
  settable via a single-byte write from the hub. Board: Arduino Nano.
*/

#include <Wire.h>

const uint8_t I2C_ADDRESS = 0x08;
const uint8_t RELAY_PINS[4] = {4, 5, 6, 7};

volatile uint8_t relayState = 0x00;

void setup() {
  Serial.begin(9600);
  for (uint8_t i = 0; i < 4; i++) {
    pinMode(RELAY_PINS[i], OUTPUT);
    digitalWrite(RELAY_PINS[i], LOW);
  }

  Wire.begin(I2C_ADDRESS);
  Wire.onRequest(onRequest);
  Wire.onReceive(onReceive);
  Serial.println("Relay bank node ready (0x08).");
}

void loop() {
  // all work happens in I2C callbacks
}

void onRequest() {
  Wire.write(relayState);
}

void onReceive(int numBytes) {
  if (numBytes < 1) return;
  uint8_t newState = Wire.read();
  relayState = newState;
  for (uint8_t i = 0; i < 4; i++) {
    digitalWrite(RELAY_PINS[i], (relayState >> i) & 0x01 ? HIGH : LOW);
  }
}
