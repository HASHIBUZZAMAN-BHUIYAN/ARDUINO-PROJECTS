/*
  Conveyor Sorting - Sorter Node (I2C slave 0x10)
  ------------------------------------------------------
  Reads a TCS3200 color sensor and swings a diverter servo to a lane
  commanded by the line controller over I2C. Board: Arduino Uno.
*/

#include <Wire.h>
#include <Servo.h>

const uint8_t I2C_ADDRESS = 0x10;
const uint8_t S0 = 2, S1 = 3, S2 = 4, S3 = 5, OUT_PIN = 6;
const uint8_t SERVO_PIN = 9;

Servo diverter;
volatile uint8_t lastColor = 0; // 0=unknown,1=red,2=green,3=blue

void setup() {
  Serial.begin(9600);
  pinMode(S0, OUTPUT); pinMode(S1, OUTPUT); pinMode(S2, OUTPUT); pinMode(S3, OUTPUT);
  pinMode(OUT_PIN, INPUT);
  digitalWrite(S0, HIGH); digitalWrite(S1, LOW); // 20% frequency scaling

  diverter.attach(SERVO_PIN);
  diverter.write(90); // straight-through

  Wire.begin(I2C_ADDRESS);
  Wire.onRequest(onRequest);
  Wire.onReceive(onReceive);
  Serial.println("Sorter node ready (0x10).");
}

void loop() {
  lastColor = detectColor();
  delay(100);
}

uint8_t detectColor() {
  digitalWrite(S2, LOW); digitalWrite(S3, LOW);
  unsigned long red = pulseIn(OUT_PIN, LOW, 30000);

  digitalWrite(S2, HIGH); digitalWrite(S3, HIGH);
  unsigned long green = pulseIn(OUT_PIN, LOW, 30000);

  digitalWrite(S2, LOW); digitalWrite(S3, HIGH);
  unsigned long blue = pulseIn(OUT_PIN, LOW, 30000);

  // TCS3200 outputs a frequency inversely proportional to intensity - a
  // SHORTER pulse means MORE of that color.
  if (red < green && red < blue) return 1;
  if (green < red && green < blue) return 2;
  if (blue < red && blue < green) return 3;
  return 0;
}

void onRequest() {
  Wire.write(lastColor);
}

void onReceive(int numBytes) {
  if (numBytes < 1) return;
  uint8_t lane = Wire.read();
  switch (lane) {
    case 1: diverter.write(45); break;  // left
    case 2: diverter.write(135); break; // right
    default: diverter.write(90); break; // straight
  }
}
