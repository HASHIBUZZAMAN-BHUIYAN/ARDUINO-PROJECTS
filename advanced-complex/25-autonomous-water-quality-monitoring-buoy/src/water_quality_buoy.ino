/*
  Autonomous Water Quality Monitoring Buoy
  -----------------------------------------------
  Fuses pH, turbidity, dissolved oxygen, water temperature, water
  level, and solar/battery power monitoring; closes the loop on an
  aerator pump when dissolved oxygen drops too fast; keeps rolling
  hourly statistics; serves a token-authenticated REST API + dashboard;
  and uses a watchdog for unattended reliability.
  Board: Arduino Uno Q.
*/

#include <OneWire.h>
#include <DallasTemperature.h>
#include <Wire.h>
#include <Adafruit_INA219.h>
#include <RTClib.h>
#include <SPI.h>
#include <SD.h>
#include <Ethernet.h>
#include <avr/wdt.h>

// ---- credentials placeholder — replace before use, never commit real secrets ----
const char *API_TOKEN = "YOUR_SHARED_SECRET_TOKEN";
// ---------------------------------------------------------------------------------

const uint8_t PH_PIN = A0, TURBIDITY_PIN = A1, DO_PIN = A2;
const uint8_t ONEWIRE_PIN = 2;
const uint8_t US_TRIG = 6, US_ECHO = 7;
const uint8_t AERATOR_PIN = 8, ALARM_PIN = 9;
const uint8_t SD_CS = 10, ETH_CS = 5;

OneWire oneWire(ONEWIRE_PIN);
DallasTemperature waterTemp(&oneWire);
Adafruit_INA219 powerSensor;
RTC_DS3231 rtc;
byte mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0x19};
IPAddress ip(192, 168, 1, 193);
EthernetServer server(80);

const float DO_DROP_RATE_ALERT = 1.0; // mg/L drop within the rolling window triggers alert
const float DO_LOW_ABSOLUTE = 3.0;

float curPh = 0, curTurbidity = 0, curDO = 0, curTempC = 0, curLevelCm = 0;
float batteryV = 0, batteryI = 0;
float doHistory[5] = {8, 8, 8, 8, 8};
uint8_t doHistIdx = 0;

bool aeratorOn = false;

struct HourlyStats { float minDO = 999, maxDO = -999, sumDO = 0; uint16_t count = 0; };
HourlyStats hourly;
int lastRollupHour = -1;

unsigned long lastSampleMs = 0;
const unsigned long SAMPLE_INTERVAL_MS = 30000; // every 30s

void setup() {
  Serial.begin(9600);
  Wire.begin();
  waterTemp.begin();
  powerSensor.begin();
  if (!rtc.begin()) Serial.println("RTC not found!");
  // First upload only: uncomment to set the RTC to your computer's clock.
  // rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));

  pinMode(AERATOR_PIN, OUTPUT); pinMode(ALARM_PIN, OUTPUT);
  digitalWrite(AERATOR_PIN, LOW); digitalWrite(ALARM_PIN, LOW);
  pinMode(US_TRIG, OUTPUT); pinMode(US_ECHO, INPUT);

  pinMode(SD_CS, OUTPUT); pinMode(ETH_CS, OUTPUT);
  digitalWrite(SD_CS, HIGH); digitalWrite(ETH_CS, HIGH);
  if (!SD.begin(SD_CS)) Serial.println("SD init failed!");
  ensureCsvHeader();

  Ethernet.init(ETH_CS);
  Ethernet.begin(mac, ip);
  server.begin();
  Serial.print("Dashboard: http://"); Serial.println(Ethernet.localIP());

  wdt_enable(WDTO_8S);
}

void loop() {
  wdt_reset();

  unsigned long now = millis();
  if (now - lastSampleMs >= SAMPLE_INTERVAL_MS) {
    lastSampleMs = now;
    readAllSensors();
    runAnomalyEngine();
    updateHourlyStats();
    checkHourlyRollup();
  }

  handleClient();
}

void readAllSensors() {
  curPh = analogToPh(analogRead(PH_PIN));
  curTurbidity = analogRead(TURBIDITY_PIN) * (5.0 / 1023.0);
  curDO = analogToDO(analogRead(DO_PIN));

  waterTemp.requestTemperatures();
  curTempC = waterTemp.getTempCByIndex(0);

  curLevelCm = readUltrasonicCm();

  batteryV = powerSensor.getBusVoltage_V();
  batteryI = powerSensor.getCurrent_mA();

  doHistory[doHistIdx] = curDO;
  doHistIdx = (doHistIdx + 1) % 5;
}

float analogToPh(int raw) {
  // linear approximation - calibrate slope/offset against pH 4/7/10 buffer solutions
  return 3.5 * (raw * (5.0 / 1023.0)) + 0.0;
}

float analogToDO(int raw) {
  // linear approximation - calibrate against a 0%/100% saturation reference
  return (raw * (5.0 / 1023.0)) * 2.5;
}

long readUltrasonicCm() {
  digitalWrite(US_TRIG, LOW); delayMicroseconds(2);
  digitalWrite(US_TRIG, HIGH); delayMicroseconds(10);
  digitalWrite(US_TRIG, LOW);
  long duration = pulseIn(US_ECHO, HIGH, 25000);
  return duration / 58;
}

void runAnomalyEngine() {
  float oldestDO = doHistory[doHistIdx]; // about to be overwritten next cycle = oldest in the ring
  float dropRate = oldestDO - curDO;
  bool anomalyDetected = (dropRate > DO_DROP_RATE_ALERT) || (curDO < DO_LOW_ABSOLUTE);

  if (anomalyDetected && !aeratorOn) {
    aeratorOn = true;
    digitalWrite(AERATOR_PIN, HIGH);
    digitalWrite(ALARM_PIN, HIGH);
    logEvent("ANOMALY_AERATOR_ENGAGED");
  } else if (!anomalyDetected && aeratorOn && curDO > DO_LOW_ABSOLUTE + 1.0) {
    aeratorOn = false;
    digitalWrite(AERATOR_PIN, LOW);
    digitalWrite(ALARM_PIN, LOW);
    logEvent("ANOMALY_CLEARED");
  }
}

void updateHourlyStats() {
  hourly.minDO = min(hourly.minDO, curDO);
  hourly.maxDO = max(hourly.maxDO, curDO);
  hourly.sumDO += curDO;
  hourly.count++;
}

void checkHourlyRollup() {
  DateTime now = rtc.now();
  if (now.hour() != lastRollupHour) {
    lastRollupHour = now.hour();
    File f = SD.open("ROLLUP.CSV", FILE_WRITE);
    if (f) {
      float avgDO = hourly.count > 0 ? hourly.sumDO / hourly.count : 0;
      f.print(now.timestamp(DateTime::TIMESTAMP_FULL)); f.print(",");
      f.print(hourly.minDO); f.print(","); f.print(hourly.maxDO); f.print(","); f.println(avgDO);
      f.close();
    }
    hourly = HourlyStats();
  }
}

void ensureCsvHeader() {
  if (!SD.exists("ROLLUP.CSV")) {
    File f = SD.open("ROLLUP.CSV", FILE_WRITE);
    if (f) { f.println("timestamp,minDO,maxDO,avgDO"); f.close(); }
  }
  if (!SD.exists("EVENTS.CSV")) {
    File f = SD.open("EVENTS.CSV", FILE_WRITE);
    if (f) { f.println("timestamp,event,ph,turbidity,do,tempC,levelCm"); f.close(); }
  }
}

void logEvent(const char *event) {
  File f = SD.open("EVENTS.CSV", FILE_WRITE);
  if (!f) return;
  DateTime now = rtc.now();
  f.print(now.timestamp(DateTime::TIMESTAMP_FULL)); f.print(",");
  f.print(event); f.print(",");
  f.print(curPh); f.print(","); f.print(curTurbidity); f.print(","); f.print(curDO); f.print(",");
  f.print(curTempC); f.print(","); f.println(curLevelCm);
  f.close();
}

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
  } else if (reqLine.indexOf("GET /api/history") >= 0) {
    sendHistory(client);
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

void sendState(EthernetClient &client) {
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: application/json");
  client.println("Connection: close");
  client.println();
  client.print("{\"ph\":"); client.print(curPh);
  client.print(",\"turbidity\":"); client.print(curTurbidity);
  client.print(",\"do\":"); client.print(curDO);
  client.print(",\"tempC\":"); client.print(curTempC);
  client.print(",\"levelCm\":"); client.print(curLevelCm);
  client.print(",\"batteryV\":"); client.print(batteryV);
  client.print(",\"batteryI\":"); client.print(batteryI);
  client.print(",\"aeratorOn\":"); client.print(aeratorOn ? "true" : "false");
  client.println("}");
}

void sendHistory(EthernetClient &client) {
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: application/json");
  client.println("Connection: close");
  client.println();

  File f = SD.open("ROLLUP.CSV");
  if (!f) { client.println("[]"); return; }
  f.readStringUntil('\n');

  const uint8_t MAX_ROWS = 24;
  String lines[MAX_ROWS];
  uint8_t count = 0;
  while (f.available()) { lines[count % MAX_ROWS] = f.readStringUntil('\n'); count++; }
  f.close();

  uint8_t start = count > MAX_ROWS ? count % MAX_ROWS : 0;
  uint8_t total = min(count, MAX_ROWS);

  client.print("[");
  for (uint8_t i = 0; i < total; i++) {
    String line = lines[(start + i) % MAX_ROWS];
    int c1 = line.indexOf(',');
    int c4 = line.lastIndexOf(',');
    String ts = line.substring(0, c1);
    String avgDO = line.substring(c4 + 1);
    if (i > 0) client.print(",");
    client.print("{\"t\":\""); client.print(ts); client.print("\",\"avgDO\":"); client.print(avgDO); client.print("}");
  }
  client.println("]");
}
