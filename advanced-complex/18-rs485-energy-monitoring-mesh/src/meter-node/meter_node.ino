/*
  RS-485 Energy Monitoring Mesh - Meter Node
  --------------------------------------------------
  Computes real power from synchronized CT clamp + voltage sensor
  samples, accumulates watt-hours, and answers hub polls over RS-485.
  Board: Arduino Nano. Set MY_ADDRESS per physical unit (1, 2, or 3).
*/

#include <SoftwareSerial.h>

const uint8_t MY_ADDRESS = 1;
const uint8_t DE_RE_PIN = 2;
const uint8_t RS485_RX = 8, RS485_TX = 9;
const uint8_t CT_PIN = A0, VOLTAGE_PIN = A1;

const float CURRENT_CAL_FACTOR = 30.0;   // amps per volt at the burden resistor - calibrate against a known load
const float VOLTAGE_CAL_FACTOR = 0.6;    // volts (mains) per volt (sensor output) - calibrate with a multimeter

SoftwareSerial rs485(RS485_RX, RS485_TX);

float cumulativeWh = 0;
unsigned long lastEnergyMs = 0;
uint8_t seq = 0;

void setup() {
  Serial.begin(9600);
  pinMode(DE_RE_PIN, OUTPUT);
  digitalWrite(DE_RE_PIN, LOW);
  rs485.begin(9600);
  lastEnergyMs = millis();
  Serial.print("Meter node "); Serial.print(MY_ADDRESS); Serial.println(" ready.");
}

void loop() {
  float watts = computeRealPower();
  accumulateEnergy(watts);

  if (rs485.available()) {
    String line = rs485.readStringUntil('\n');
    line.trim();
    if (line.startsWith("POLL")) {
      int addr = line.substring(5).toInt();
      if (addr == MY_ADDRESS) respondWithReading(watts);
    }
  }
}

float computeRealPower() {
  // Sample both channels together over ~1 mains cycle (20ms @ 50Hz) and
  // average their instantaneous product - this correctly handles any
  // voltage/current phase offset, unlike multiplying separate RMS values.
  const uint16_t SAMPLES = 200;
  double sumPower = 0;
  int vOffset = 512, iOffset = 512; // assume mid-supply bias; refine via calibration

  for (uint16_t i = 0; i < SAMPLES; i++) {
    int vRaw = analogRead(VOLTAGE_PIN) - vOffset;
    int iRaw = analogRead(CT_PIN) - iOffset;
    float vInstant = vRaw * VOLTAGE_CAL_FACTOR;
    float iInstant = iRaw * (CURRENT_CAL_FACTOR / 512.0);
    sumPower += vInstant * iInstant;
  }
  float avgPower = sumPower / SAMPLES;
  return abs(avgPower);
}

void accumulateEnergy(float watts) {
  unsigned long now = millis();
  float hoursElapsed = (now - lastEnergyMs) / 3600000.0;
  lastEnergyMs = now;
  cumulativeWh += watts * hoursElapsed;
}

void respondWithReading(float watts) {
  String payload = "D," + String(MY_ADDRESS) + "," + String(watts, 1) + "," + String(cumulativeWh, 2) + "," + String(seq);
  uint8_t chk = checksum(payload);
  String frame = payload + "," + String(chk);

  digitalWrite(DE_RE_PIN, HIGH);
  delay(2);
  rs485.println(frame);
  rs485.flush();
  digitalWrite(DE_RE_PIN, LOW);

  seq++;
  Serial.print("Replied: "); Serial.println(frame);
}

uint8_t checksum(const String &s) {
  uint16_t sum = 0;
  for (uint16_t i = 0; i < s.length(); i++) sum += (uint8_t)s[i];
  return sum % 256;
}
