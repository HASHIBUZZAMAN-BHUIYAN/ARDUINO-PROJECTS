/*
  RS-485 Energy Monitoring Mesh - Hub Controller
  ------------------------------------------------------
  Polls 3 meter nodes over RS-485, logs to SD with an RTC timestamp,
  and serves a JSON API + dashboard over Ethernet.
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
byte mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0x08};
IPAddress ip(192, 168, 1, 187);
EthernetServer server(80);

struct Circuit {
  bool online = false;
  float watts = 0;
  float wh = 0;
};
Circuit circuits[4]; // index 1..3 used

uint8_t currentAddr = 1;
unsigned long lastPollMs = 0;
const unsigned long POLL_INTERVAL_MS = 5000 / 3;

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
    pollNode(currentAddr);
    currentAddr = (currentAddr % 3) + 1;
  }
  handleClient();
}

void pollNode(uint8_t addr) {
  String reply = sendAndWait(addr, 200);
  if (reply.length() == 0) reply = sendAndWait(addr, 200);

  if (reply.length() == 0) {
    circuits[addr].online = false;
    return;
  }
  if (!parseAndValidate(reply, addr)) {
    circuits[addr].online = false;
    return;
  }
  circuits[addr].online = true;
  logRow(addr);
}

String sendAndWait(uint8_t addr, unsigned long timeoutMs) {
  digitalWrite(DE_RE_PIN, HIGH);
  delay(2);
  rs485.print("POLL "); rs485.println(addr);
  rs485.flush();
  digitalWrite(DE_RE_PIN, LOW);

  unsigned long start = millis();
  while (millis() - start < timeoutMs) {
    if (rs485.available()) {
      String line = rs485.readStringUntil('\n');
      line.trim();
      if (line.startsWith("D,")) return line;
    }
  }
  return "";
}

bool parseAndValidate(const String &line, uint8_t expectedAddr) {
  int lastComma = line.lastIndexOf(',');
  String payload = line.substring(0, lastComma);
  uint8_t receivedChk = line.substring(lastComma + 1).toInt();
  if (checksum(payload) != receivedChk) return false;

  int c1 = line.indexOf(',');
  int c2 = line.indexOf(',', c1 + 1);
  int c3 = line.indexOf(',', c2 + 1);
  int addr = line.substring(c1 + 1, c2).toInt();
  if (addr != expectedAddr) return false;

  circuits[addr].watts = line.substring(c2 + 1, c3).toFloat();
  circuits[addr].wh = line.substring(c3 + 1, line.lastIndexOf(',', line.length())).toFloat();
  return true;
}

uint8_t checksum(const String &s) {
  uint16_t sum = 0;
  for (uint16_t i = 0; i < s.length(); i++) sum += (uint8_t)s[i];
  return sum % 256;
}

void ensureCsvHeader() {
  if (!SD.exists("ENERGYLOG.CSV")) {
    File f = SD.open("ENERGYLOG.CSV", FILE_WRITE);
    if (f) { f.println("timestamp,circuit,watts,wh"); f.close(); }
  }
}

void logRow(uint8_t addr) {
  File f = SD.open("ENERGYLOG.CSV", FILE_WRITE);
  if (!f) return;
  DateTime now = rtc.now();
  f.print(now.timestamp(DateTime::TIMESTAMP_FULL)); f.print(",");
  f.print(addr); f.print(",");
  f.print(circuits[addr].watts); f.print(",");
  f.println(circuits[addr].wh);
  f.close();
}

void handleClient() {
  EthernetClient client = server.available();
  if (!client) return;

  String reqLine = client.readStringUntil('\r');
  client.flush();

  if (reqLine.indexOf("GET /api/circuits") >= 0) {
    sendCircuits(client);
  } else {
    client.println("HTTP/1.1 404 Not Found");
    client.println("Connection: close");
    client.println();
  }
  delay(1);
  client.stop();
}

void sendCircuits(EthernetClient &client) {
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: application/json");
  client.println("Connection: close");
  client.println();

  client.print("[");
  for (uint8_t addr = 1; addr <= 3; addr++) {
    if (addr > 1) client.print(",");
    client.print("{\"id\":"); client.print(addr);
    client.print(",\"watts\":"); client.print(circuits[addr].watts);
    client.print(",\"whToday\":"); client.print(circuits[addr].wh);
    client.print(",\"online\":"); client.print(circuits[addr].online ? "true" : "false");
    client.print("}");
  }
  client.println("]");
}
