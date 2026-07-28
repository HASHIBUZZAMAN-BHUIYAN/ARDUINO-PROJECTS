/*
  RS-485 Sensor Mesh - Node 1 "Climate"
  ----------------------------------------
  Answers hub polls on the shared RS-485 bus with DHT22 temperature/
  humidity and BH1750 light readings. Board: Arduino Uno.
*/

#include <SoftwareSerial.h>
#include <Wire.h>
#include <DHT.h>
#include <BH1750.h>

const uint8_t MY_ADDRESS = 1;
const uint8_t DE_RE_PIN = 2;
const uint8_t RS485_RX = 8, RS485_TX = 9;
const uint8_t DHT_PIN = 3;

SoftwareSerial rs485(RS485_RX, RS485_TX);
DHT dht(DHT_PIN, DHT22);
BH1750 lightMeter;

uint8_t seq = 0;

void setup() {
  Serial.begin(9600);
  dht.begin();
  Wire.begin();
  lightMeter.begin();

  pinMode(DE_RE_PIN, OUTPUT);
  digitalWrite(DE_RE_PIN, LOW); // listen mode
  rs485.begin(9600);
  Serial.println("Node 1 (Climate) ready.");
}

void loop() {
  if (rs485.available()) {
    String line = rs485.readStringUntil('\n');
    line.trim();
    if (line.startsWith("POLL")) {
      int addr = line.substring(5).toInt();
      if (addr == MY_ADDRESS) respondWithReading();
    }
  }
}

void respondWithReading() {
  float temp = dht.readTemperature();
  float lux = lightMeter.readLightLevel();
  if (isnan(temp)) temp = -999;

  String payload = "D," + String(MY_ADDRESS) + "," + String(temp, 1) + "," + String(lux, 0) + "," + String(seq);
  uint8_t chk = checksum(payload);
  String frame = payload + "," + String(chk);

  digitalWrite(DE_RE_PIN, HIGH); // transmit mode
  delay(2);
  rs485.println(frame);
  rs485.flush();
  digitalWrite(DE_RE_PIN, LOW); // back to listen mode

  seq++;
  Serial.print("Replied: "); Serial.println(frame);
}

uint8_t checksum(const String &s) {
  uint16_t sum = 0;
  for (uint16_t i = 0; i < s.length(); i++) sum += (uint8_t)s[i];
  return sum % 256;
}
