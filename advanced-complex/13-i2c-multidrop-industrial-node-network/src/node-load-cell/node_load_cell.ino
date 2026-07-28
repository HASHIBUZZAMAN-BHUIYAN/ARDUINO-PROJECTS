/*
  I2C Multi-Drop Network - Load Cell Node (0x09)
  ---------------------------------------------------
  I2C slave exposing the latest HX711 weight reading as a float.
  Board: Arduino Nano.
*/

#include <Wire.h>
#include <HX711.h>

const uint8_t I2C_ADDRESS = 0x09;
const uint8_t HX_DOUT = 2, HX_SCK = 3;
const float CALIBRATION_FACTOR = 2280.0; // calibrate against a known weight

HX711 scale;
volatile float latestWeight = 0;

void setup() {
  Serial.begin(9600);
  scale.begin(HX_DOUT, HX_SCK);
  scale.set_scale(CALIBRATION_FACTOR);
  scale.tare();

  Wire.begin(I2C_ADDRESS);
  Wire.onRequest(onRequest);
  Serial.println("Load cell node ready (0x09).");
}

void loop() {
  if (scale.is_ready()) {
    latestWeight = scale.get_units(3);
  }
  delay(200);
}

void onRequest() {
  Wire.write((uint8_t *)&latestWeight, sizeof(float));
}
