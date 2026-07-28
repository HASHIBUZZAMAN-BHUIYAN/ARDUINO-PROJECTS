/*
  I2C Multi-Drop Network - Hub Controller
  ---------------------------------------------
  I2C master polling 3 Nano slave nodes, logging to SD with an RTC
  timestamp, and serving a JSON API + dashboard over Ethernet.
  Board: Arduino Uno Q.
*/

#include <Wire.h>
#include <SPI.h>
#include <RTClib.h>
#include <SD.h>
#include <Ethernet.h>

const uint8_t ADDR_RELAY = 0x08, ADDR_LOADCELL = 0x09, ADDR_THERMO = 0x0A;
const uint8_t SD_CS = 9, ETH_CS = 7;

RTC_DS3231 rtc;
byte mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0x06};
IPAddress ip(192, 168, 1, 185);
EthernetServer server(80);

uint8_t relayState = 0;
float weight = 0;
float tempC = 0;

unsigned long lastPollMs = 0;
const unsigned long POLL_INTERVAL_MS = 2000;

void setup() {
  Serial.begin(9600);
  Wire.begin(); // master mode

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
    pollAllNodes();
    logRow();
  }
  handleClient();
}

void pollAllNodes() {
  Wire.requestFrom(ADDR_RELAY, (uint8_t)1);
  if (Wire.available()) relayState = Wire.read();

  Wire.requestFrom(ADDR_LOADCELL, (uint8_t)sizeof(float));
  if (Wire.available() >= (int)sizeof(float)) Wire.readBytes((char *)&weight, sizeof(float));

  Wire.requestFrom(ADDR_THERMO, (uint8_t)sizeof(float));
  if (Wire.available() >= (int)sizeof(float)) Wire.readBytes((char *)&tempC, sizeof(float));
}

void setRelay(uint8_t bitIndex, bool on) {
  if (on) relayState |= (1 << bitIndex);
  else relayState &= ~(1 << bitIndex);

  Wire.beginTransmission(ADDR_RELAY);
  Wire.write(relayState);
  Wire.endTransmission();
}

void ensureCsvHeader() {
  if (!SD.exists("NODENET.CSV")) {
    File f = SD.open("NODENET.CSV", FILE_WRITE);
    if (f) { f.println("timestamp,relayState,weight,tempC"); f.close(); }
  }
}

void logRow() {
  File f = SD.open("NODENET.CSV", FILE_WRITE);
  if (!f) return;
  DateTime now = rtc.now();
  f.print(now.timestamp(DateTime::TIMESTAMP_FULL)); f.print(",");
  f.print(relayState); f.print(",");
  f.print(weight); f.print(",");
  f.println(tempC);
  f.close();
}

void handleClient() {
  EthernetClient client = server.available();
  if (!client) return;

  String reqLine = client.readStringUntil('\r');
  client.flush();

  if (reqLine.indexOf("GET /api/nodes") >= 0) {
    sendNodes(client);
  } else if (reqLine.indexOf("/api/relay/") >= 0) {
    handleRelayToggle(reqLine);
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

void handleRelayToggle(const String &reqLine) {
  int idx = reqLine.indexOf("/api/relay/");
  int bit = reqLine.substring(idx + 11).toInt();
  bool on = reqLine.indexOf("/on") >= 0;
  if (bit >= 0 && bit < 4) setRelay(bit, on);
}

void sendNodes(EthernetClient &client) {
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: application/json");
  client.println("Connection: close");
  client.println();
  client.print("{\"relayState\":"); client.print(relayState);
  client.print(",\"weight\":"); client.print(weight);
  client.print(",\"tempC\":"); client.print(tempC);
  client.println("}");
}
