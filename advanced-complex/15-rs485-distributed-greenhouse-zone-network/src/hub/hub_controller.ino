/*
  RS-485 Distributed Greenhouse - Hub Controller
  ------------------------------------------------------
  Supervisory-only: polls 3 zone controllers over RS-485, logs to SD,
  and serves a JSON API + dashboard for monitoring and setpoint pushes.
  Board: Arduino Mega 2560.
*/

#include <SoftwareSerial.h>
#include <Wire.h>
#include <SPI.h>
#include <RTClib.h>
#include <SD.h>
#include <Ethernet.h>

const uint8_t DE_RE_PIN = 2;
const uint8_t RS485_RX = 8, RS485_TX = 9;
const uint8_t SD_CS = 53, ETH_CS = 49;

SoftwareSerial rs485(RS485_RX, RS485_TX);
RTC_DS3231 rtc;
byte mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0x07};
IPAddress ip(192, 168, 1, 186);
EthernetServer server(80);

struct Zone {
  bool online = false;
  float temp = 0, hum = 0, soil = 0;
  bool heater = false, fan = false, mist = false;
};
Zone zones[4]; // index 1..3 used

uint8_t currentZone = 1;
unsigned long lastPollMs = 0;
const unsigned long POLL_INTERVAL_MS = 10000 / 3;

void setup() {
  Serial.begin(9600);
  Wire.begin();

  pinMode(DE_RE_PIN, OUTPUT);
  digitalWrite(DE_RE_PIN, LOW);
  rs485.begin(9600);

  if (!rtc.begin()) Serial.println("RTC not found!");

  pinMode(SD_CS, OUTPUT); pinMode(ETH_CS, OUTPUT);
  digitalWrite(SD_CS, HIGH); digitalWrite(ETH_CS, HIGH);
  if (!SD.begin(SD_CS)) Serial.println("SD init failed!");
  ensureCsvHeader();

  Ethernet.init(ETH_CS);
  Ethernet.begin(mac, ip);
  server.begin();
  Serial.print("Dashboard: http://");
  Serial.println(Ethernet.localIP());
}

void loop() {
  unsigned long now = millis();
  if (now - lastPollMs >= POLL_INTERVAL_MS) {
    lastPollMs = now;
    pollZone(currentZone);
    currentZone = (currentZone % 3) + 1;
  }
  handleClient();
}

void pollZone(uint8_t zoneId) {
  String reply = sendAndWait("POLL " + String(zoneId), 200);
  if (reply.length() == 0) reply = sendAndWait("POLL " + String(zoneId), 200);

  if (reply.length() == 0) {
    zones[zoneId].online = false;
    Serial.print("Zone "); Serial.print(zoneId); Serial.println(" OFFLINE.");
    return;
  }
  if (!parseAndValidate(reply, zoneId)) {
    zones[zoneId].online = false;
    return;
  }
  zones[zoneId].online = true;
  logRow(zoneId);
}

void pushSetpoint(uint8_t zoneId, float t, float h) {
  String cmd = "SET " + String(zoneId) + " " + String(t, 1) + " " + String(h, 1);
  sendAndWait(cmd, 200);
}

String sendAndWait(const String &cmd, unsigned long timeoutMs) {
  digitalWrite(DE_RE_PIN, HIGH);
  delay(2);
  rs485.println(cmd);
  rs485.flush();
  digitalWrite(DE_RE_PIN, LOW);

  unsigned long start = millis();
  while (millis() - start < timeoutMs) {
    if (rs485.available()) {
      String line = rs485.readStringUntil('\n');
      line.trim();
      if (line.length() > 0) return line;
    }
  }
  return "";
}

bool parseAndValidate(const String &line, uint8_t expectedZone) {
  int lastComma = line.lastIndexOf(',');
  String payload = line.substring(0, lastComma);
  uint8_t receivedChk = line.substring(lastComma + 1).toInt();
  if (checksum(payload) != receivedChk) return false;

  int c[7];
  c[0] = line.indexOf(',');
  for (uint8_t i = 1; i < 7; i++) c[i] = line.indexOf(',', c[i - 1] + 1);

  int zoneId = line.substring(c[0] + 1, c[1]).toInt();
  if (zoneId != expectedZone) return false;

  zones[zoneId].temp = line.substring(c[1] + 1, c[2]).toFloat();
  zones[zoneId].hum = line.substring(c[2] + 1, c[3]).toFloat();
  zones[zoneId].soil = line.substring(c[3] + 1, c[4]).toFloat();
  zones[zoneId].heater = line.substring(c[4] + 1, c[5]).toInt();
  zones[zoneId].fan = line.substring(c[5] + 1, c[6]).toInt();
  zones[zoneId].mist = line.substring(c[6] + 1).toInt();
  return true;
}

uint8_t checksum(const String &s) {
  uint16_t sum = 0;
  for (uint16_t i = 0; i < s.length(); i++) sum += (uint8_t)s[i];
  return sum % 256;
}

void ensureCsvHeader() {
  if (!SD.exists("ZONELOG.CSV")) {
    File f = SD.open("ZONELOG.CSV", FILE_WRITE);
    if (f) { f.println("timestamp,zone,temp,hum,soil,heater,fan,mist"); f.close(); }
  }
}

void logRow(uint8_t zoneId) {
  File f = SD.open("ZONELOG.CSV", FILE_WRITE);
  if (!f) return;
  DateTime now = rtc.now();
  Zone &z = zones[zoneId];
  f.print(now.timestamp(DateTime::TIMESTAMP_FULL)); f.print(",");
  f.print(zoneId); f.print(",");
  f.print(z.temp); f.print(",");
  f.print(z.hum); f.print(",");
  f.print(z.soil); f.print(",");
  f.print(z.heater); f.print(",");
  f.print(z.fan); f.print(",");
  f.println(z.mist);
  f.close();
}

void handleClient() {
  EthernetClient client = server.available();
  if (!client) return;

  String reqLine = client.readStringUntil('\r');
  client.flush();

  if (reqLine.indexOf("GET /api/zones") >= 0 && reqLine.indexOf("setpoint") < 0) {
    sendZones(client);
  } else if (reqLine.indexOf("/setpoint") >= 0) {
    handleSetpointPush(reqLine);
    client.println("HTTP/1.1 200 OK");
    client.println("Connection: close");
    client.println();
  } else {
    client.println("HTTP/1.1 404 Not Found");
    client.println("Connection: close");
    client.println();
  }
  delay(1);
  client.stop();
}

void handleSetpointPush(const String &reqLine) {
  int idx = reqLine.indexOf("/api/zones/");
  int zoneId = reqLine.substring(idx + 11).toInt();
  int tIdx = reqLine.indexOf("t=");
  int hIdx = reqLine.indexOf("h=");
  if (zoneId < 1 || zoneId > 3 || tIdx < 0 || hIdx < 0) return;
  float t = reqLine.substring(tIdx + 2).toFloat();
  float h = reqLine.substring(hIdx + 2).toFloat();
  pushSetpoint(zoneId, t, h);
}

void sendZones(EthernetClient &client) {
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: application/json");
  client.println("Connection: close");
  client.println();

  client.print("[");
  for (uint8_t z = 1; z <= 3; z++) {
    if (z > 1) client.print(",");
    client.print("{\"zone\":"); client.print(z);
    client.print(",\"online\":"); client.print(zones[z].online ? "true" : "false");
    client.print(",\"temp\":"); client.print(zones[z].temp);
    client.print(",\"hum\":"); client.print(zones[z].hum);
    client.print(",\"soil\":"); client.print(zones[z].soil);
    client.print(",\"heater\":"); client.print(zones[z].heater ? 1 : 0);
    client.print(",\"fan\":"); client.print(zones[z].fan ? 1 : 0);
    client.print(",\"mist\":"); client.print(zones[z].mist ? 1 : 0);
    client.print("}");
  }
  client.println("]");
}
