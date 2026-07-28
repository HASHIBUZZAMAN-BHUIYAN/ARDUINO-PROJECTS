/*
  RS-485 Sensor Mesh - Node 3 "Occupancy"
  -------------------------------------------
  Answers hub polls with PIR motion and door reed-switch state.
  Board: Arduino Uno.
*/

#include <SoftwareSerial.h>

const uint8_t MY_ADDRESS = 3;
const uint8_t DE_RE_PIN = 2;
const uint8_t RS485_RX = 8, RS485_TX = 9;
const uint8_t PIR_PIN = 4, REED_PIN = 5;

SoftwareSerial rs485(RS485_RX, RS485_TX);
uint8_t seq = 0;

void setup() {
  Serial.begin(9600);
  pinMode(PIR_PIN, INPUT);
  pinMode(REED_PIN, INPUT_PULLUP);
  pinMode(DE_RE_PIN, OUTPUT);
  digitalWrite(DE_RE_PIN, LOW);
  rs485.begin(9600);
  Serial.println("Node 3 (Occupancy) ready.");
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
  int motion = digitalRead(PIR_PIN);
  int doorOpen = digitalRead(REED_PIN) == HIGH ? 1 : 0; // pulled low when closed

  String payload = "D," + String(MY_ADDRESS) + "," + String(motion) + "," + String(doorOpen) + "," + String(seq);
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
