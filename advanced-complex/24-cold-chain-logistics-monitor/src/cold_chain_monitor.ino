/*
  Cold Chain Logistics Monitor
  -----------------------------------
  Fuses 3x DS18B20 + DHT22 + ADXL345 + door/tamper switches, buffers
  every reading/event to SD, and syncs pending rows over a SIM800L
  cellular module (plain-HTTP AT commands) with store-and-forward
  reliability. A sustained excursion also closes a local cooling loop
  independent of connectivity. Board: Arduino Mega 2560.
*/

#include <OneWire.h>
#include <DallasTemperature.h>
#include <DHT.h>
#include <Wire.h>
#include <Adafruit_ADXL345_U.h>
#include <RTClib.h>
#include <SPI.h>
#include <SD.h>

// ---- credentials placeholder — replace before use, never commit real secrets ----
const char* APN = "YOUR_CARRIER_APN";
const char* INGEST_URL = "http://your-ingest-endpoint.example.com/coldchain";
const char* BEARER_TOKEN = "YOUR_BEARER_TOKEN";
// ---------------------------------------------------------------------------------

const uint8_t ONEWIRE_PIN = 2;
const uint8_t DHT_PIN = 3;
const uint8_t DOOR_PIN = 4, TAMPER_PIN = 5;
const uint8_t COOLING_RELAY_PIN = 6, BUZZER_PIN = 7;
const uint8_t SD_CS = 53;

const float TEMP_EXCURSION_MAX_C = 8.0; // e.g. refrigerated goods upper limit
const float SHOCK_THRESHOLD_G = 2.5;

OneWire oneWire(ONEWIRE_PIN);
DallasTemperature tempSensors(&oneWire);
DHT dht(DHT_PIN, DHT22);
Adafruit_ADXL345_Unified accel = Adafruit_ADXL345_Unified(12345);
RTC_DS3231 rtc;

unsigned long lastSampleMs = 0;
const unsigned long SAMPLE_INTERVAL_MS = 60000; // periodic snapshot every minute
unsigned long lastSyncAttemptMs = 0;
const unsigned long SYNC_RETRY_INTERVAL_MS = 30000;

bool coolingActive = false;
unsigned long excursionSinceMs = 0;

void setup() {
  Serial.begin(9600);      // USB debug
  Serial2.begin(9600);     // SIM800L link
  tempSensors.begin();
  dht.begin();
  Wire.begin();
  accel.begin();
  accel.setRange(ADXL345_RANGE_4_G);
  if (!rtc.begin()) Serial.println("RTC not found!");

  pinMode(DOOR_PIN, INPUT_PULLUP);
  pinMode(TAMPER_PIN, INPUT_PULLUP);
  pinMode(COOLING_RELAY_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(COOLING_RELAY_PIN, LOW);
  digitalWrite(BUZZER_PIN, LOW);

  if (!SD.begin(SD_CS)) Serial.println("SD init failed!");
  ensureCsvHeader();

  initSim800();
}

void loop() {
  unsigned long now = millis();

  if (now - lastSampleMs >= SAMPLE_INTERVAL_MS) {
    lastSampleMs = now;
    takeSnapshotAndCheckExcursions();
  }

  checkDoorAndTamperEvents();

  if (now - lastSyncAttemptMs >= SYNC_RETRY_INTERVAL_MS) {
    lastSyncAttemptMs = now;
    syncPendingRows();
  }
}

// ---- sensing ----
struct Reading {
  float t1, t2, t3, hum, shockG;
  bool door, tamper;
};

Reading readAll() {
  Reading r;
  tempSensors.requestTemperatures();
  r.t1 = tempSensors.getTempCByIndex(0);
  r.t2 = tempSensors.getTempCByIndex(1);
  r.t3 = tempSensors.getTempCByIndex(2);
  r.hum = dht.readHumidity();

  sensors_event_t event;
  accel.getEvent(&event);
  r.shockG = sqrt(sq(event.acceleration.x) + sq(event.acceleration.y) + sq(event.acceleration.z)) / 9.8;

  r.door = digitalRead(DOOR_PIN) == HIGH;
  r.tamper = digitalRead(TAMPER_PIN) == HIGH;
  return r;
}

void takeSnapshotAndCheckExcursions() {
  Reading r = readAll();
  logRow(r, "SNAPSHOT");

  float maxTemp = max(r.t1, max(r.t2, r.t3));
  bool excursion = maxTemp > TEMP_EXCURSION_MAX_C;

  if (excursion) {
    if (excursionSinceMs == 0) excursionSinceMs = millis();
    if (millis() - excursionSinceMs > 120000 && !coolingActive) { // sustained 2 min
      coolingActive = true;
      digitalWrite(COOLING_RELAY_PIN, HIGH);
      digitalWrite(BUZZER_PIN, HIGH);
      logRow(r, "EXCURSION_COOLING_ENGAGED");
    }
  } else {
    excursionSinceMs = 0;
    if (coolingActive) {
      coolingActive = false;
      digitalWrite(COOLING_RELAY_PIN, LOW);
      digitalWrite(BUZZER_PIN, LOW);
      logRow(r, "EXCURSION_CLEARED");
    }
  }

  if (r.shockG > SHOCK_THRESHOLD_G) logRow(r, "SHOCK_EVENT");
}

bool lastDoorState = false, lastTamperState = false;

void checkDoorAndTamperEvents() {
  Reading r = readAll();
  if (r.door != lastDoorState) {
    lastDoorState = r.door;
    logRow(r, r.door ? "DOOR_OPENED" : "DOOR_CLOSED");
  }
  if (r.tamper != lastTamperState) {
    lastTamperState = r.tamper;
    logRow(r, r.tamper ? "TAMPER_TRIPPED" : "TAMPER_CLEARED");
  }
}

// ---- SD store-and-forward buffer ----
void ensureCsvHeader() {
  if (!SD.exists("COLDLOG.CSV")) {
    File f = SD.open("COLDLOG.CSV", FILE_WRITE);
    if (f) { f.println("timestamp,event,t1,t2,t3,hum,shockG,door,tamper,synced"); f.close(); }
  }
}

void logRow(const Reading &r, const char *event) {
  File f = SD.open("COLDLOG.CSV", FILE_WRITE);
  if (!f) return;
  DateTime now = rtc.now();
  f.print(now.timestamp(DateTime::TIMESTAMP_FULL)); f.print(",");
  f.print(event); f.print(",");
  f.print(r.t1); f.print(","); f.print(r.t2); f.print(","); f.print(r.t3); f.print(",");
  f.print(r.hum); f.print(","); f.print(r.shockG); f.print(",");
  f.print(r.door); f.print(","); f.print(r.tamper); f.print(",");
  f.println(0); // synced=0 initially
  f.close();
}

void syncPendingRows() {
  if (!ensureSim800Connected()) {
    Serial.println("No cellular connectivity - rows remain buffered.");
    return;
  }

  File f = SD.open("COLDLOG.CSV");
  if (!f) return;
  f.readStringUntil('\n'); // skip header

  while (f.available()) {
    String row = f.readStringUntil('\n');
    row.trim();
    if (row.length() == 0) continue;
    if (row.endsWith(",0")) {
      if (sendRowOverCellular(row)) {
        markRowSynced(row);
      } else {
        break; // stop on first failure, retry from here next pass
      }
    }
  }
  f.close();
}

void markRowSynced(const String &originalRow) {
  // Rewrite the file, flipping the matched row's trailing ",0" to ",1".
  // A microSD card at this data volume handles a full rewrite fine; a
  // production system with a much larger log would use an index file
  // instead of a linear rewrite.
  File src = SD.open("COLDLOG.CSV");
  File tmp = SD.open("COLDTMP.CSV", FILE_WRITE);
  if (!src || !tmp) return;

  while (src.available()) {
    String line = src.readStringUntil('\n');
    if (line == originalRow) {
      line.remove(line.length() - 1);
      line += "1";
    }
    tmp.println(line);
  }
  src.close();
  tmp.close();
  SD.remove("COLDLOG.CSV");
  tmp = SD.open("COLDTMP.CSV");
  File dst = SD.open("COLDLOG.CSV", FILE_WRITE);
  while (tmp.available()) dst.write(tmp.read());
  tmp.close();
  dst.close();
  SD.remove("COLDTMP.CSV");
}

// ---- SIM800L (plain AT-command HTTP flow) ----
void initSim800() {
  sendAtCommand("AT", 1000);
  sendAtCommand("AT+CPIN?", 1000);
  String apnCmd = "AT+SAPBR=3,1,\"APN\",\"" + String(APN) + "\"";
  sendAtCommand(apnCmd, 1000);
}

bool ensureSim800Connected() {
  String resp = sendAtCommand("AT+CREG?", 1000);
  return resp.indexOf(",1") >= 0 || resp.indexOf(",5") >= 0; // registered (home or roaming)
}

bool sendRowOverCellular(const String &csvRow) {
  String json = csvRowToJson(csvRow);

  sendAtCommand("AT+SAPBR=1,1", 3000);      // open GPRS bearer
  sendAtCommand("AT+HTTPINIT", 1000);
  sendAtCommand("AT+HTTPPARA=\"CID\",1", 500);
  sendAtCommand("AT+HTTPPARA=\"URL\",\"" + String(INGEST_URL) + "\"", 500);
  sendAtCommand("AT+HTTPPARA=\"CONTENT\",\"application/json\"", 500);
  sendAtCommand("AT+HTTPPARA=\"USERDATA\",\"Authorization: Bearer " + String(BEARER_TOKEN) + "\"", 500);

  String lenCmd = "AT+HTTPDATA=" + String(json.length()) + ",5000";
  sendAtCommand(lenCmd, 500);
  Serial2.print(json);
  delay(500);

  String actionResp = sendAtCommand("AT+HTTPACTION=1", 5000); // POST
  sendAtCommand("AT+HTTPTERM", 500);

  // A successful POST reports a 2xx status in the +HTTPACTION URC, e.g. "1,200,123"
  return actionResp.indexOf(",200,") >= 0;
}

String csvRowToJson(const String &row) {
  int c[9];
  c[0] = row.indexOf(',');
  for (uint8_t i = 1; i < 9; i++) c[i] = row.indexOf(',', c[i - 1] + 1);

  String ts = row.substring(0, c[0]);
  String t1 = row.substring(c[1] + 1, c[2]);
  String t2 = row.substring(c[2] + 1, c[3]);
  String t3 = row.substring(c[3] + 1, c[4]);
  String hum = row.substring(c[4] + 1, c[5]);
  String shock = row.substring(c[5] + 1, c[6]);
  String door = row.substring(c[6] + 1, c[7]);
  String tamper = row.substring(c[7] + 1, c[8]);

  String json = "{\"ts\":\"" + ts + "\",\"t1\":" + t1 + ",\"t2\":" + t2 + ",\"t3\":" + t3 +
                ",\"hum\":" + hum + ",\"shockG\":" + shock + ",\"door\":" + door + ",\"tamper\":" + tamper + "}";
  return json;
}

String sendAtCommand(const String &cmd, unsigned long waitMs) {
  Serial2.println(cmd);
  unsigned long start = millis();
  String resp = "";
  while (millis() - start < waitMs) {
    while (Serial2.available()) resp += (char)Serial2.read();
  }
  Serial.print("AT> "); Serial.print(cmd); Serial.print(" -> "); Serial.println(resp);
  return resp;
}
