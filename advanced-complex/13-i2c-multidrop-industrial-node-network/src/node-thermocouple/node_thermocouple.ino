/*
  I2C Multi-Drop Network - Thermocouple Node (0x0A)
  ------------------------------------------------------
  I2C slave exposing the latest MAX6675 K-type temperature reading.
  Board: Arduino Nano.
*/

#include <Wire.h>
#include <max6675.h>

const uint8_t I2C_ADDRESS = 0x0A;
const uint8_t TC_CS = 8, TC_SO = 11, TC_SCK = 13;

MAX6675 thermocouple(TC_SCK, TC_CS, TC_SO);
volatile float latestTempC = 0;

void setup() {
  Serial.begin(9600);
  delay(500); // MAX6675 needs time to stabilize after power-up

  Wire.begin(I2C_ADDRESS);
  Wire.onRequest(onRequest);
  Serial.println("Thermocouple node ready (0x0A).");
}

void loop() {
  latestTempC = thermocouple.readCelsius();
  delay(250); // MAX6675 conversion takes ~220ms
}

void onRequest() {
  Wire.write((uint8_t *)&latestTempC, sizeof(float));
}
