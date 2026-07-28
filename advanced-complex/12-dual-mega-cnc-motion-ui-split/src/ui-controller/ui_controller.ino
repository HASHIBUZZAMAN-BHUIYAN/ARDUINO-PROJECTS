/*
  Dual-Board CNC - UI Controller
  -----------------------------------
  Reads G-code from SD, shows status on a 16x2 LCD, and feeds one
  motion packet at a time to the motion controller over SoftwareSerial,
  waiting for ACK before sending the next line.
  Board: Arduino Uno.
*/

#include <SPI.h>
#include <SD.h>
#include <LiquidCrystal.h>
#include <SoftwareSerial.h>

const uint8_t SD_CS = 9;
SoftwareSerial link(4, 10); // RX, TX - crossed to Mega's Serial2
LiquidCrystal lcd(2, 3, 5, 6, 7, 8);

#pragma pack(push, 1)
struct MovePacket {
  uint8_t start = 0x7E;
  uint8_t cmdId;
  int16_t x, y, z;   // tenths of mm
  uint16_t feedRate;
  uint8_t crc8;
  uint8_t end = 0x7F;
};
#pragma pack(pop)

File program;
uint8_t nextCmdId = 1;
float curX = 0, curY = 0, curZ = 0;

void setup() {
  Serial.begin(9600);   // USB debug console
  link.begin(115200);

  lcd.begin(16, 2);
  lcd.print("CNC UI ready");

  pinMode(SD_CS, OUTPUT);
  if (!SD.begin(SD_CS)) {
    lcd.setCursor(0, 1);
    lcd.print("SD init failed");
  }
}

void loop() {
  if (Serial.available()) {
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();
    if (cmd.equalsIgnoreCase("RUN")) runProgram();
  }
  drainStatusLines();
}

void runProgram() {
  program = SD.open("PROGRAM.GCO");
  if (!program) {
    lcd.setCursor(0, 1);
    lcd.print("No PROGRAM.GCO");
    return;
  }

  lcd.clear();
  lcd.print("Running...");

  while (program.available()) {
    String line = program.readStringUntil('\n');
    line.trim();
    if (line.length() == 0) continue;

    MovePacket pkt = parseLine(line);
    sendAndWaitForAck(pkt);
  }
  program.close();

  lcd.clear();
  lcd.print("Program done");
}

MovePacket parseLine(const String &line) {
  MovePacket pkt;
  pkt.cmdId = nextCmdId++;
  pkt.x = curX * 10; pkt.y = curY * 10; pkt.z = curZ * 10;
  pkt.feedRate = 800;

  if (line.startsWith("G28")) {
    pkt.cmdId = 0; // reserved home command
    return pkt;
  }

  int xi = line.indexOf('X'), yi = line.indexOf('Y'), zi = line.indexOf('Z'), fi = line.indexOf('F');
  if (xi >= 0) curX = line.substring(xi + 1).toFloat();
  if (yi >= 0) curY = line.substring(yi + 1).toFloat();
  if (zi >= 0) curZ = line.substring(zi + 1).toFloat();
  if (fi >= 0) pkt.feedRate = line.substring(fi + 1).toFloat();

  pkt.x = curX * 10; pkt.y = curY * 10; pkt.z = curZ * 10;
  return pkt;
}

void sendAndWaitForAck(MovePacket pkt) {
  pkt.crc8 = crc8((uint8_t *)&pkt, sizeof(MovePacket) - 2);
  link.write((uint8_t *)&pkt, sizeof(MovePacket));

  unsigned long start = millis();
  while (millis() - start < 30000) { // generous timeout for long moves/homing
    if (link.available()) {
      String reply = link.readStringUntil('\n');
      if (reply.startsWith("ACK")) return;
    }
  }
  Serial.println("Timed out waiting for ACK - check motion controller link.");
}

void drainStatusLines() {
  if (!link.available()) return;
  String line = link.readStringUntil('\n');
  if (line.startsWith("POS,")) {
    lcd.setCursor(0, 1);
    lcd.print(line.substring(4, min((int)line.length(), 16)));
  }
}

uint8_t crc8(const uint8_t *data, size_t len) {
  uint8_t crc = 0;
  for (size_t i = 0; i < len; i++) {
    crc ^= data[i];
    for (uint8_t b = 0; b < 8; b++) {
      crc = (crc & 0x80) ? (crc << 1) ^ 0x07 : (crc << 1);
    }
  }
  return crc;
}
