/*
  Multiplexed 8-Zone Soil Irrigation Scheduler
  -----------------------------------------------
  CD74HC4067 multiplexes 8 soil probes onto one analog pin; RTC-scheduled,
  rain-adaptive watering with closed-loop moisture-threshold shutoff.
  Served over a W5500 REST API. Board: Arduino Mega 2560.
*/

#include <Wire.h>
#include <SPI.h>
#include <RTClib.h>
#include <SD.h>
#include <Ethernet.h>

const uint8_t MUX_SIG = A0;
const uint8_t MUX_SEL[4] = {22, 23, 24, 25};
const uint8_t RAIN_PIN = 2;
const uint8_t RELAY_PINS[8] = {30, 31, 32, 33, 34, 35, 36, 37};
const uint8_t SD_CS = 53, ETH_CS = 49;

RTC_DS3231 rtc;
byte mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0x03};
IPAddress ip(192, 168, 1, 182);
EthernetServer server(80);

struct Zone {
  uint8_t startHour, startMinute;
  int moistureThreshold;      // stop watering once analog reading is BELOW this (wetter = lower with capacitive probes wired this way)
  unsigned long maxRuntimeMs;
  bool valveOpen = false;
  unsigned long openedAtMs = 0;
  int lastSoil = 0;
  bool wateredToday = false;
};

Zone zones[8] = {
  {6, 0, 500, 300000UL, false, 0, 0, false},
  {6, 5, 500, 300000UL, false, 0, 0, false},
  {6, 10, 500, 300000UL, false, 0, 0, false},
  {6, 15, 500, 300000UL, false, 0, 0, false},
  {18, 0, 500, 300000UL, false, 0, 0, false},
  {18, 5, 500, 300000UL, false, 0, 0, false},
  {18, 10, 500, 300000UL, false, 0, 0, false},
  {18, 15, 500, 300000UL, false, 0, 0, false},
};

unsigned long lastPollMs = 0;
const unsigned long POLL_INTERVAL_MS = 3000;

void setup() {
  Serial.begin(9600);
  Wire.begin();
  for (uint8_t i = 0; i < 4; i++) pinMode(MUX_SEL[i], OUTPUT);
  pinMode(RAIN_PIN, INPUT_PULLUP);
  for (uint8_t i = 0; i < 8; i++) { pinMode(RELAY_PINS[i], OUTPUT); digitalWrite(RELAY_PINS[i], HIGH); } // active-low relay boards: HIGH = off

  if (!rtc.begin()) Serial.println("RTC not found!");
  // First upload only: uncomment to set the RTC to your computer's clock.
  // rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));

  pinMode(SD_CS, OUTPUT);
  pinMode(ETH_CS, OUTPUT);
  digitalWrite(SD_CS, HIGH);
  digitalWrite(ETH_CS, HIGH);
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
    for (uint8_t z = 0; z < 8; z++) {
      zones[z].lastSoil = readZone(z);
    }
    checkSchedules();
    checkClosedLoopShutoff();
  }
  handleClient();
}

int readZone(uint8_t zone) {
  for (uint8_t bit = 0; bit < 4; bit++) {
    digitalWrite(MUX_SEL[bit], (zone >> bit) & 0x01);
  }
  delayMicroseconds(50); // settle time
  return analogRead(MUX_SIG);
}

bool isRaining() {
  return digitalRead(RAIN_PIN) == LOW;
}

void checkSchedules() {
  DateTime now = rtc.now();
  if (now.hour() == 0 && now.minute() == 0) {
    for (uint8_t z = 0; z < 8; z++) zones[z].wateredToday = false;
  }
  for (uint8_t z = 0; z < 8; z++) {
    Zone &zone = zones[z];
    bool windowNow = (now.hour() == zone.startHour && now.minute() == zone.startMinute);
    if (windowNow && !zone.valveOpen && !zone.wateredToday) {
      if (isRaining()) {
        Serial.print("Zone "); Serial.print(z); Serial.println(" skipped: rain detected.");
        zone.wateredToday = true;
      } else {
        openValve(z);
      }
    }
  }
}

void checkClosedLoopShutoff() {
  unsigned long now = millis();
  for (uint8_t z = 0; z < 8; z++) {
    Zone &zone = zones[z];
    if (!zone.valveOpen) continue;
    bool moistureReached = zone.lastSoil < zone.moistureThreshold;
    bool timedOut = (now - zone.openedAtMs) > zone.maxRuntimeMs;
    if (moistureReached || timedOut) {
      closeValve(z, timedOut ? "timeout" : "moisture-target");
    }
  }
}

void openValve(uint8_t z) {
  zones[z].valveOpen = true;
  zones[z].openedAtMs = millis();
  zones[z].wateredToday = true;
  digitalWrite(RELAY_PINS[z], LOW); // active-low: LOW = on
  logEvent(z, "OPEN", "scheduled");
}

void closeValve(uint8_t z, const char *reason) {
  zones[z].valveOpen = false;
  digitalWrite(RELAY_PINS[z], HIGH);
  logEvent(z, "CLOSE", reason);
}

void ensureCsvHeader() {
  if (!SD.exists("IRRIGLOG.CSV")) {
    File f = SD.open("IRRIGLOG.CSV", FILE_WRITE);
    if (f) { f.println("timestamp,zone,event,reason,soil"); f.close(); }
  }
}

void logEvent(uint8_t z, const char *event, const char *reason) {
  File f = SD.open("IRRIGLOG.CSV", FILE_WRITE);
  if (!f) return;
  DateTime now = rtc.now();
  f.print(now.timestamp(DateTime::TIMESTAMP_FULL)); f.print(",");
  f.print(z); f.print(",");
  f.print(event); f.print(",");
  f.print(reason); f.print(",");
  f.println(zones[z].lastSoil);
  f.close();
}

void handleClient() {
  EthernetClient client = server.available();
  if (!client) return;

  String reqLine = client.readStringUntil('\r');
  client.flush();

  if (reqLine.indexOf("GET /api/zones") >= 0) {
    sendZones(client);
  } else if (reqLine.indexOf("/api/zones/") >= 0) {
    handleOverride(reqLine);
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

void handleOverride(const String &reqLine) {
  int idx = reqLine.indexOf("/api/zones/");
  int zoneNum = reqLine.substring(idx + 11).toInt();
  if (zoneNum < 0 || zoneNum > 7) return;
  if (reqLine.indexOf("/on") >= 0) openValve(zoneNum);
  else if (reqLine.indexOf("/off") >= 0) closeValve(zoneNum, "manual-override");
}

void sendZones(EthernetClient &client) {
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: application/json");
  client.println("Connection: close");
  client.println();

  client.print("[");
  for (uint8_t z = 0; z < 8; z++) {
    if (z > 0) client.print(",");
    client.print("{\"zone\":"); client.print(z);
    client.print(",\"soil\":"); client.print(zones[z].lastSoil);
    client.print(",\"valveOpen\":"); client.print(zones[z].valveOpen ? 1 : 0);
    client.print(",\"startHour\":"); client.print(zones[z].startHour);
    client.print(",\"startMinute\":"); client.print(zones[z].startMinute);
    client.print("}");
  }
  client.println("]");
}
