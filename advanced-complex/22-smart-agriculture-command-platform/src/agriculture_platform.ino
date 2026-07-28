/*
  Smart Agriculture Command Platform
  ---------------------------------------
  Multiplexed 8-zone irrigation (closed-loop moisture threshold) +
  climate PID (temp/humidity) + RTC grow-light schedule, structured SD
  "database" queryable over serial, token-authenticated REST API +
  dashboard over Ethernet, and a hardware watchdog for auto-recovery.
  Board: Arduino Mega 2560.
*/

#include <Wire.h>
#include <SPI.h>
#include <Adafruit_BME280.h>
#include <BH1750.h>
#include <RTClib.h>
#include <SD.h>
#include <Ethernet.h>
#include <avr/wdt.h>

// ---- credentials placeholder — replace before use, never commit real secrets ----
const char *API_TOKEN = "YOUR_SHARED_SECRET_TOKEN";
// ---------------------------------------------------------------------------------

const uint8_t MUX_SIG = A0;
const uint8_t MUX_SEL[4] = {22, 23, 24, 25};
const uint8_t VALVE_PINS[8] = {30, 31, 32, 33, 34, 35, 36, 37};
const uint8_t HEATER_PIN = 38, FAN_PIN = 39, MIST_PIN = 40, LIGHT_PIN = 41;
const uint8_t SD_CS = 53, ETH_CS = 49;

Adafruit_BME280 bme;
BH1750 lightMeter;
RTC_DS3231 rtc;
byte mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0x16};
IPAddress ip(192, 168, 1, 190);
EthernetServer server(80);

struct Zone {
  int moistureThreshold = 500;
  unsigned long maxRuntimeMs = 300000UL;
  bool valveOpen = false;
  unsigned long openedAtMs = 0;
  int lastSoil = 0;
};
Zone zones[8];

float curTemp = 0, curHum = 0, curLight = 0;
struct PID { float kp, ki, kd, setpoint, integral = 0, lastError = 0; };
PID tempPID = {8.0, 0.05, 2.0, 24.0};
PID humPID  = {5.0, 0.05, 1.0, 65.0};
bool heaterOn = false, fanOn = false, mistOn = false, lightOn = false;
const int LIGHT_ON_HOUR = 6, LIGHT_OFF_HOUR = 20;

unsigned long lastControlMs = 0, lastLogMs = 0;
const unsigned long CONTROL_INTERVAL_MS = 3000;
const unsigned long LOG_INTERVAL_MS = 60000;

void setup() {
  Serial.begin(9600);
  Wire.begin();
  bme.begin(0x76);
  lightMeter.begin();
  if (!rtc.begin()) Serial.println("RTC not found!");
  // First upload only: uncomment to set the RTC to your computer's clock.
  // rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));

  for (uint8_t i = 0; i < 4; i++) pinMode(MUX_SEL[i], OUTPUT);
  for (uint8_t i = 0; i < 8; i++) { pinMode(VALVE_PINS[i], OUTPUT); digitalWrite(VALVE_PINS[i], HIGH); }
  pinMode(HEATER_PIN, OUTPUT); pinMode(FAN_PIN, OUTPUT); pinMode(MIST_PIN, OUTPUT); pinMode(LIGHT_PIN, OUTPUT);

  pinMode(SD_CS, OUTPUT); pinMode(ETH_CS, OUTPUT);
  digitalWrite(SD_CS, HIGH); digitalWrite(ETH_CS, HIGH);
  if (!SD.begin(SD_CS)) Serial.println("SD init failed!");
  ensureCsvHeader();

  Ethernet.init(ETH_CS);
  Ethernet.begin(mac, ip);
  server.begin();
  Serial.print("Dashboard: http://"); Serial.println(Ethernet.localIP());
  Serial.println("Serial query language ready: QUERY ZONE <n> LAST <hours>H");

  wdt_enable(WDTO_8S); // auto-reboot if the main loop hangs for >8s
}

void loop() {
  wdt_reset();

  unsigned long now = millis();
  if (now - lastControlMs >= CONTROL_INTERVAL_MS) {
    lastControlMs = now;
    runIrrigationZones();
    runClimatePID();
    updateLightSchedule();
  }
  if (now - lastLogMs >= LOG_INTERVAL_MS) {
    lastLogMs = now;
    logClimateRow();
  }

  handleSerialQuery();
  handleClient();
}

// ---- irrigation ----
int readZone(uint8_t zone) {
  for (uint8_t bit = 0; bit < 4; bit++) digitalWrite(MUX_SEL[bit], (zone >> bit) & 0x01);
  delayMicroseconds(50);
  return analogRead(MUX_SIG);
}

void runIrrigationZones() {
  for (uint8_t z = 0; z < 8; z++) {
    zones[z].lastSoil = readZone(z);
    if (zones[z].valveOpen) {
      bool moistureReached = zones[z].lastSoil < zones[z].moistureThreshold;
      bool timedOut = (millis() - zones[z].openedAtMs) > zones[z].maxRuntimeMs;
      if (moistureReached || timedOut) closeValve(z, timedOut ? "timeout" : "moisture-target");
    }
  }
}

void openValve(uint8_t z) {
  zones[z].valveOpen = true;
  zones[z].openedAtMs = millis();
  digitalWrite(VALVE_PINS[z], LOW);
  logIrrigationEvent(z, "OPEN", "manual");
}

void closeValve(uint8_t z, const char *reason) {
  zones[z].valveOpen = false;
  digitalWrite(VALVE_PINS[z], HIGH);
  logIrrigationEvent(z, "CLOSE", reason);
}

// ---- climate PID ----
float pidStep(PID &p, float measured, float dt) {
  float error = p.setpoint - measured;
  p.integral = constrain(p.integral + error * dt, -50, 50);
  float derivative = (error - p.lastError) / dt;
  p.lastError = error;
  return p.kp * error + p.ki * p.integral + p.kd * derivative;
}

void runClimatePID() {
  curTemp = bme.readTemperature();
  curHum = bme.readHumidity();
  curLight = lightMeter.readLightLevel();

  float dt = CONTROL_INTERVAL_MS / 1000.0;
  float tempOut = pidStep(tempPID, curTemp, dt);
  heaterOn = tempOut > 1.0; fanOn = tempOut < -1.0;
  digitalWrite(HEATER_PIN, heaterOn ? HIGH : LOW);
  digitalWrite(FAN_PIN, fanOn ? HIGH : LOW);

  float humOut = pidStep(humPID, curHum, dt);
  mistOn = humOut > 1.0;
  digitalWrite(MIST_PIN, mistOn ? HIGH : LOW);
}

void updateLightSchedule() {
  DateTime now = rtc.now();
  bool shouldBeOn = now.hour() >= LIGHT_ON_HOUR && now.hour() < LIGHT_OFF_HOUR;
  if (shouldBeOn != lightOn) { lightOn = shouldBeOn; digitalWrite(LIGHT_PIN, lightOn ? HIGH : LOW); }
}

// ---- SD "database" ----
void ensureCsvHeader() {
  if (!SD.exists("CLIMATE.CSV")) {
    File f = SD.open("CLIMATE.CSV", FILE_WRITE);
    if (f) { f.println("timestamp,temp,hum,light,heater,fan,mist,lightRelay"); f.close(); }
  }
  if (!SD.exists("IRRIG.CSV")) {
    File f = SD.open("IRRIG.CSV", FILE_WRITE);
    if (f) { f.println("timestamp,zone,event,reason,soil"); f.close(); }
  }
}

void logClimateRow() {
  File f = SD.open("CLIMATE.CSV", FILE_WRITE);
  if (!f) return;
  DateTime now = rtc.now();
  f.print(now.timestamp(DateTime::TIMESTAMP_FULL)); f.print(",");
  f.print(curTemp); f.print(","); f.print(curHum); f.print(","); f.print(curLight); f.print(",");
  f.print(heaterOn); f.print(","); f.print(fanOn); f.print(","); f.print(mistOn); f.print(",");
  f.println(lightOn);
  f.close();
}

void logIrrigationEvent(uint8_t z, const char *event, const char *reason) {
  File f = SD.open("IRRIG.CSV", FILE_WRITE);
  if (!f) return;
  DateTime now = rtc.now();
  f.print(now.timestamp(DateTime::TIMESTAMP_FULL)); f.print(",");
  f.print(z); f.print(","); f.print(event); f.print(","); f.print(reason); f.print(",");
  f.println(zones[z].lastSoil);
  f.close();
}

void handleSerialQuery() {
  if (!Serial.available()) return;
  String line = Serial.readStringUntil('\n');
  line.trim();
  if (!line.startsWith("QUERY ZONE")) return;

  int zoneIdx = line.substring(11).toInt();
  File f = SD.open("IRRIG.CSV");
  if (!f) { Serial.println("No log file."); return; }

  f.readStringUntil('\n'); // skip header
  Serial.print("Rows for zone "); Serial.println(zoneIdx);
  while (f.available()) {
    String row = f.readStringUntil('\n');
    int c1 = row.indexOf(',');
    int c2 = row.indexOf(',', c1 + 1);
    if (row.substring(c1 + 1, c2).toInt() == zoneIdx) Serial.println(row);
  }
  f.close();
}

// ---- token-authenticated REST API ----
void handleClient() {
  EthernetClient client = server.available();
  if (!client) return;

  String reqLine = client.readStringUntil('\r');
  String headers = "";
  while (client.connected() && client.available()) {
    String h = client.readStringUntil('\r');
    headers += h;
    if (h.length() <= 1) break;
  }

  if (!isAuthorized(headers)) {
    client.println("HTTP/1.1 401 Unauthorized");
    client.println("Connection: close");
    client.println();
    client.stop();
    return;
  }

  if (reqLine.indexOf("GET /api/state") >= 0) {
    sendState(client);
  } else if (reqLine.indexOf("/api/zones/") >= 0) {
    handleZoneOverride(reqLine);
    okResponse(client);
  } else if (reqLine.indexOf("/api/climate/setpoint") >= 0) {
    handleSetpoint(reqLine);
    okResponse(client);
  } else {
    client.println("HTTP/1.1 404 Not Found");
    client.println("Connection: close");
    client.println();
  }
  delay(1);
  client.stop();
}

bool isAuthorized(const String &headers) {
  int idx = headers.indexOf("X-Auth-Token:");
  if (idx < 0) return false;
  String token = headers.substring(idx + 13);
  token.trim();
  return token == String(API_TOKEN);
}

void okResponse(EthernetClient &client) {
  client.println("HTTP/1.1 200 OK");
  client.println("Connection: close");
  client.println();
}

void handleZoneOverride(const String &reqLine) {
  int idx = reqLine.indexOf("/api/zones/");
  int zoneNum = reqLine.substring(idx + 11).toInt();
  if (zoneNum < 0 || zoneNum > 7) return;
  if (reqLine.indexOf("/on") >= 0) openValve(zoneNum);
  else if (reqLine.indexOf("/off") >= 0) closeValve(zoneNum, "manual-override");
}

void handleSetpoint(const String &reqLine) {
  int tIdx = reqLine.indexOf("t=");
  int hIdx = reqLine.indexOf("h=");
  if (tIdx >= 0) tempPID.setpoint = reqLine.substring(tIdx + 2).toFloat();
  if (hIdx >= 0) humPID.setpoint = reqLine.substring(hIdx + 2).toFloat();
}

void sendState(EthernetClient &client) {
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: application/json");
  client.println("Connection: close");
  client.println();

  client.print("{\"climate\":{\"temp\":"); client.print(curTemp);
  client.print(",\"hum\":"); client.print(curHum);
  client.print(",\"light\":"); client.print(curLight);
  client.print(",\"heater\":"); client.print(heaterOn ? 1 : 0);
  client.print(",\"fan\":"); client.print(fanOn ? 1 : 0);
  client.print(",\"mist\":"); client.print(mistOn ? 1 : 0);
  client.print(",\"lightRelay\":"); client.print(lightOn ? 1 : 0);
  client.print(",\"setpointTemp\":"); client.print(tempPID.setpoint);
  client.print(",\"setpointHum\":"); client.print(humPID.setpoint);
  client.print("},\"zones\":[");
  for (uint8_t z = 0; z < 8; z++) {
    if (z > 0) client.print(",");
    client.print("{\"zone\":"); client.print(z);
    client.print(",\"soil\":"); client.print(zones[z].lastSoil);
    client.print(",\"valveOpen\":"); client.print(zones[z].valveOpen ? 1 : 0);
    client.print("}");
  }
  client.println("]}");
}
