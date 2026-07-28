/*
  Multi-Sensor Weather Buoy Datalogger
  ----------------------------------------
  Fuses anemometer, wind vane, rain gauge, BME280, UV, and ultrasonic
  water-level sensors into a 5-minute SD log with daily rollups, served
  over a W5500 JSON API. Board: Arduino Uno Q.
*/

#include <Wire.h>
#include <SPI.h>
#include <Adafruit_BME280.h>
#include <RTClib.h>
#include <SD.h>
#include <Ethernet.h>

const uint8_t ANEMOMETER_PIN = 2;
const uint8_t RAIN_PIN = 3;
const uint8_t VANE_PIN = A0;
const uint8_t UV_PIN = A1;
const uint8_t US_TRIG = 6, US_ECHO = 7;
const uint8_t SD_CS = 10, ETH_CS = 9;

const float ANEMOMETER_KPH_PER_PULSE_HZ = 2.4; // datasheet-specific calibration
const float RAIN_MM_PER_TIP = 0.2794;

Adafruit_BME280 bme;
RTC_DS3231 rtc;
byte mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0x04};
IPAddress ip(192, 168, 1, 183);
EthernetServer server(80);

volatile unsigned long windPulses = 0;
volatile unsigned long rainPulses = 0;

struct Snapshot {
  float windKph = 0, rainMmToday = 0, temp = 0, hum = 0, pressure = 0, uv = 0, waterCm = 0;
  const char *windDir = "N";
} current;

struct DailyStats {
  float minTemp = 999, maxTemp = -999, sumTemp = 0;
  uint16_t count = 0;
} daily;

unsigned long lastSnapshotMs = 0;
const unsigned long SNAPSHOT_INTERVAL_MS = 300000UL; // 5 min
int lastLoggedDay = -1;

void windISR() { windPulses++; }
void rainISR() { rainPulses++; }

void setup() {
  Serial.begin(9600);
  Wire.begin();

  pinMode(ANEMOMETER_PIN, INPUT_PULLUP);
  pinMode(RAIN_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(ANEMOMETER_PIN), windISR, FALLING);
  attachInterrupt(digitalPinToInterrupt(RAIN_PIN), rainISR, FALLING);

  pinMode(US_TRIG, OUTPUT);
  pinMode(US_ECHO, INPUT);

  if (!bme.begin(0x76)) Serial.println("BME280 not found!");
  if (!rtc.begin()) Serial.println("RTC not found!");
  // First upload only: uncomment to set the RTC to your computer's clock.
  // rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));

  pinMode(SD_CS, OUTPUT); pinMode(ETH_CS, OUTPUT);
  digitalWrite(SD_CS, HIGH); digitalWrite(ETH_CS, HIGH);
  if (!SD.begin(SD_CS)) Serial.println("SD init failed!");
  ensureCsvHeader();

  Ethernet.init(ETH_CS);
  Ethernet.begin(mac, ip);
  server.begin();
  Serial.print("Dashboard: http://");
  Serial.println(Ethernet.localIP());

  lastSnapshotMs = millis();
}

void loop() {
  unsigned long now = millis();
  if (now - lastSnapshotMs >= SNAPSHOT_INTERVAL_MS) {
    lastSnapshotMs = now;
    buildSnapshot();
    updateDailyStats();
    logRow();
    checkMidnightRollup();
  }
  handleClient();
}

void buildSnapshot() {
  noInterrupts();
  unsigned long wp = windPulses; windPulses = 0;
  unsigned long rp = rainPulses; rainPulses = 0;
  interrupts();

  float pulseHz = wp / (SNAPSHOT_INTERVAL_MS / 1000.0);
  current.windKph = pulseHz * ANEMOMETER_KPH_PER_PULSE_HZ;
  current.rainMmToday += rp * RAIN_MM_PER_TIP;

  int vaneRaw = analogRead(VANE_PIN);
  current.windDir = vaneToDirection(vaneRaw);

  current.temp = bme.readTemperature();
  current.hum = bme.readHumidity();
  current.pressure = bme.readPressure() / 100.0F;
  current.uv = analogRead(UV_PIN) * (5.0 / 1023.0);
  current.waterCm = readUltrasonicCm();
}

const char *vaneToDirection(int raw) {
  static const char *dirs[8] = {"N", "NE", "E", "SE", "S", "SW", "W", "NW"};
  uint8_t idx = map(raw, 0, 1023, 0, 7);
  return dirs[idx];
}

long readUltrasonicCm() {
  digitalWrite(US_TRIG, LOW); delayMicroseconds(2);
  digitalWrite(US_TRIG, HIGH); delayMicroseconds(10);
  digitalWrite(US_TRIG, LOW);
  long duration = pulseIn(US_ECHO, HIGH, 25000);
  return duration / 58;
}

void updateDailyStats() {
  daily.minTemp = min(daily.minTemp, current.temp);
  daily.maxTemp = max(daily.maxTemp, current.temp);
  daily.sumTemp += current.temp;
  daily.count++;
}

void checkMidnightRollup() {
  DateTime now = rtc.now();
  if (now.hour() == 0 && now.day() != lastLoggedDay) {
    lastLoggedDay = now.day();
    logDailySummary(now);
    daily = DailyStats();
    current.rainMmToday = 0;
  }
}

void ensureCsvHeader() {
  if (!SD.exists("WEATHER.CSV")) {
    File f = SD.open("WEATHER.CSV", FILE_WRITE);
    if (f) { f.println("timestamp,windKph,windDir,rainMmToday,temp,hum,pressure,uv,waterCm"); f.close(); }
  }
}

void logRow() {
  File f = SD.open("WEATHER.CSV", FILE_WRITE);
  if (!f) return;
  DateTime now = rtc.now();
  f.print(now.timestamp(DateTime::TIMESTAMP_FULL)); f.print(",");
  f.print(current.windKph); f.print(",");
  f.print(current.windDir); f.print(",");
  f.print(current.rainMmToday); f.print(",");
  f.print(current.temp); f.print(",");
  f.print(current.hum); f.print(",");
  f.print(current.pressure); f.print(",");
  f.print(current.uv); f.print(",");
  f.println(current.waterCm);
  f.close();
}

void logDailySummary(DateTime &now) {
  File f = SD.open("WEATHER_DAILY.CSV", FILE_WRITE);
  if (!f) return;
  float avgTemp = daily.count > 0 ? daily.sumTemp / daily.count : 0;
  f.print(now.timestamp(DateTime::TIMESTAMP_DATE)); f.print(",");
  f.print(daily.minTemp); f.print(",");
  f.print(daily.maxTemp); f.print(",");
  f.println(avgTemp);
  f.close();
}

void handleClient() {
  EthernetClient client = server.available();
  if (!client) return;

  String reqLine = client.readStringUntil('\r');
  client.flush();

  if (reqLine.indexOf("GET /api/history") >= 0) {
    sendHistory(client);
  } else if (reqLine.indexOf("GET /api/data") >= 0) {
    sendData(client);
  } else {
    client.println("HTTP/1.1 404 Not Found");
    client.println("Connection: close");
    client.println();
  }
  delay(1);
  client.stop();
}

void sendData(EthernetClient &client) {
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: application/json");
  client.println("Connection: close");
  client.println();
  client.print("{\"windKph\":"); client.print(current.windKph);
  client.print(",\"windDir\":\""); client.print(current.windDir);
  client.print("\",\"rainMmToday\":"); client.print(current.rainMmToday);
  client.print(",\"temp\":"); client.print(current.temp);
  client.print(",\"hum\":"); client.print(current.hum);
  client.print(",\"pressure\":"); client.print(current.pressure);
  client.print(",\"uv\":"); client.print(current.uv);
  client.print(",\"waterCm\":"); client.print(current.waterCm);
  client.println("}");
}

void sendHistory(EthernetClient &client) {
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: application/json");
  client.println("Connection: close");
  client.println();

  File f = SD.open("WEATHER.CSV");
  if (!f) { client.println("[]"); return; }

  const uint8_t MAX_ROWS = 24;
  String lines[MAX_ROWS];
  uint8_t count = 0;
  f.readStringUntil('\n');
  while (f.available()) { lines[count % MAX_ROWS] = f.readStringUntil('\n'); count++; }
  f.close();

  uint8_t start = count > MAX_ROWS ? count % MAX_ROWS : 0;
  uint8_t total = min(count, MAX_ROWS);

  client.print("[");
  for (uint8_t i = 0; i < total; i++) {
    String line = lines[(start + i) % MAX_ROWS];
    int c1 = line.indexOf(',');
    int c2 = line.indexOf(',', c1 + 1);
    String ts = line.substring(0, c1);
    String wind = line.substring(c1 + 1, c2);
    if (i > 0) client.print(",");
    client.print("{\"t\":\""); client.print(ts); client.print("\",\"windKph\":"); client.print(wind); client.print("}");
  }
  client.println("]");
}
