/*
  Home Energy Resilience Management System
  ---------------------------------------------
  Monitors grid/solar/battery power via 3 INA219 sensors, automatically
  prefers solar > battery > grid, sheds non-critical loads as SoC drops,
  persists state through power loss via EEPROM, logs daily rollups to
  SD, and serves a token-authenticated REST API + dashboard.
  Board: Arduino Mega 2560.
*/

#include <Wire.h>
#include <SPI.h>
#include <Adafruit_INA219.h>
#include <RTClib.h>
#include <SD.h>
#include <Ethernet.h>
#include <EEPROM.h>

// ---- credentials placeholder — replace before use, never commit real secrets ----
const char *API_TOKEN = "YOUR_SHARED_SECRET_TOKEN";
// ---------------------------------------------------------------------------------

Adafruit_INA219 gridSensor(0x40);
Adafruit_INA219 solarSensor(0x41);
Adafruit_INA219 batterySensor(0x44);
RTC_DS3231 rtc;

const uint8_t RELAY_GRID_IN = 30, RELAY_SOLAR_BATT_IN = 31;
const uint8_t RELAY_SHED_1 = 32, RELAY_SHED_2 = 33;
const uint8_t SD_CS = 53, ETH_CS = 49;

byte mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0x17};
IPAddress ip(192, 168, 1, 191);
EthernetServer server(80);

const float BATTERY_CAPACITY_AH = 100.0;
const float SHED1_THRESHOLD = 40.0; // %
const float SHED2_THRESHOLD = 20.0;
const float RESTORE1_THRESHOLD = 55.0; // hysteresis
const float RESTORE2_THRESHOLD = 35.0;

struct PersistState {
  uint16_t magic;
  bool usingGrid;
  bool shed1Active, shed2Active;
  float stateOfCharge;
};
const uint16_t MAGIC = 0xE573;
PersistState state;

float gridW = 0, solarW = 0, batteryW = 0;
unsigned long lastControlMs = 0, lastRollupCheckMs = 0;
const unsigned long CONTROL_INTERVAL_MS = 2000;
int lastRollupDay = -1;

void setup() {
  Serial.begin(9600);
  Wire.begin();
  gridSensor.begin(); solarSensor.begin(); batterySensor.begin();
  if (!rtc.begin()) Serial.println("RTC not found!");

  pinMode(RELAY_GRID_IN, OUTPUT); pinMode(RELAY_SOLAR_BATT_IN, OUTPUT);
  pinMode(RELAY_SHED_1, OUTPUT); pinMode(RELAY_SHED_2, OUTPUT);

  loadOrInitState();
  applyRelayState();

  pinMode(SD_CS, OUTPUT); pinMode(ETH_CS, OUTPUT);
  digitalWrite(SD_CS, HIGH); digitalWrite(ETH_CS, HIGH);
  if (!SD.begin(SD_CS)) Serial.println("SD init failed!");
  ensureCsvHeader();

  Ethernet.init(ETH_CS);
  Ethernet.begin(mac, ip);
  server.begin();
  Serial.print("Dashboard: http://"); Serial.println(Ethernet.localIP());
}

void loop() {
  unsigned long now = millis();
  if (now - lastControlMs >= CONTROL_INTERVAL_MS) {
    lastControlMs = now;
    readSensors();
    updateSoC(CONTROL_INTERVAL_MS / 1000.0);
    selectSource();
    updateLoadShedding();
    checkDailyRollup();
  }
  handleClient();
}

void loadOrInitState() {
  EEPROM.get(0, state);
  if (state.magic != MAGIC) {
    state.magic = MAGIC;
    state.usingGrid = true;
    state.shed1Active = false;
    state.shed2Active = false;
    state.stateOfCharge = 80.0;
    saveState();
  }
}

void saveState() { EEPROM.put(0, state); }

void readSensors() {
  gridW = gridSensor.getBusVoltage_V() * (gridSensor.getCurrent_mA() / 1000.0);
  solarW = solarSensor.getBusVoltage_V() * (solarSensor.getCurrent_mA() / 1000.0);
  batteryW = batterySensor.getBusVoltage_V() * (batterySensor.getCurrent_mA() / 1000.0);
}

void updateSoC(float dtSeconds) {
  float ampHoursDelta = (batterySensor.getCurrent_mA() / 1000.0) * (dtSeconds / 3600.0);
  bool changed = abs(ampHoursDelta) > 0.0001;
  state.stateOfCharge = constrain(state.stateOfCharge + (ampHoursDelta / BATTERY_CAPACITY_AH) * 100.0, 0, 100);
  if (changed) saveState(); // persist immediately so a power loss doesn't lose SoC tracking
}

void selectSource() {
  bool solarSufficient = solarW > 50.0; // watts - tune to your loads
  bool wantGrid = !solarSufficient && state.stateOfCharge < 15.0;

  if (wantGrid != state.usingGrid) {
    state.usingGrid = wantGrid;
    applyRelayState();
    saveState();
    Serial.println(wantGrid ? "Switched to GRID." : "Switched to SOLAR/BATTERY.");
  }
}

void applyRelayState() {
  digitalWrite(RELAY_GRID_IN, state.usingGrid ? HIGH : LOW);
  digitalWrite(RELAY_SOLAR_BATT_IN, state.usingGrid ? LOW : HIGH); // mutually exclusive
  digitalWrite(RELAY_SHED_1, state.shed1Active ? LOW : HIGH);      // active-low relay assumed
  digitalWrite(RELAY_SHED_2, state.shed2Active ? LOW : HIGH);
}

void updateLoadShedding() {
  bool changed = false;
  if (!state.shed1Active && state.stateOfCharge < SHED1_THRESHOLD) { state.shed1Active = true; changed = true; }
  else if (state.shed1Active && state.stateOfCharge > RESTORE1_THRESHOLD) { state.shed1Active = false; changed = true; }

  if (!state.shed2Active && state.stateOfCharge < SHED2_THRESHOLD) { state.shed2Active = true; changed = true; }
  else if (state.shed2Active && state.stateOfCharge > RESTORE2_THRESHOLD) { state.shed2Active = false; changed = true; }

  if (changed) {
    applyRelayState();
    saveState();
    Serial.print("Load shed state changed: 1="); Serial.print(state.shed1Active);
    Serial.print(" 2="); Serial.println(state.shed2Active);
  }
}

void ensureCsvHeader() {
  if (!SD.exists("ENERGYROLLUP.CSV")) {
    File f = SD.open("ENERGYROLLUP.CSV", FILE_WRITE);
    if (f) { f.println("date,gridMinW,gridMaxW,gridAvgW,solarAvgW,batteryAvgW,socEnd"); f.close(); }
  }
}

float gridMin = 999999, gridMax = -999999, gridSum = 0, solarSum = 0, battSum = 0;
uint16_t rollupSamples = 0;

void checkDailyRollup() {
  gridMin = min(gridMin, gridW); gridMax = max(gridMax, gridW);
  gridSum += gridW; solarSum += solarW; battSum += batteryW;
  rollupSamples++;

  DateTime now = rtc.now();
  if (now.hour() == 0 && now.day() != lastRollupDay) {
    lastRollupDay = now.day();
    File f = SD.open("ENERGYROLLUP.CSV", FILE_WRITE);
    if (f) {
      f.print(now.timestamp(DateTime::TIMESTAMP_DATE)); f.print(",");
      f.print(gridMin); f.print(","); f.print(gridMax); f.print(",");
      f.print(gridSum / max(rollupSamples, (uint16_t)1)); f.print(",");
      f.print(solarSum / max(rollupSamples, (uint16_t)1)); f.print(",");
      f.print(battSum / max(rollupSamples, (uint16_t)1)); f.print(",");
      f.println(state.stateOfCharge);
      f.close();
    }
    gridMin = 999999; gridMax = -999999; gridSum = 0; solarSum = 0; battSum = 0; rollupSamples = 0;
  }
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
  client.print("{\"gridW\":"); client.print(gridW);
  client.print(",\"solarW\":"); client.print(solarW);
  client.print(",\"batteryW\":"); client.print(batteryW);
  client.print(",\"soc\":"); client.print(state.stateOfCharge);
  client.print(",\"usingGrid\":"); client.print(state.usingGrid ? "true" : "false");
  client.print(",\"shed1\":"); client.print(state.shed1Active ? "true" : "false");
  client.print(",\"shed2\":"); client.print(state.shed2Active ? "true" : "false");
  client.println("}");
}

void sendHistory(EthernetClient &client) {
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: application/json");
  client.println("Connection: close");
  client.println();

  File f = SD.open("ENERGYROLLUP.CSV");
  if (!f) { client.println("[]"); return; }
  f.readStringUntil('\n');

  client.print("[");
  bool first = true;
  while (f.available()) {
    String row = f.readStringUntil('\n');
    row.trim();
    if (row.length() == 0) continue;
    int c1 = row.indexOf(',');
    String date = row.substring(0, c1);
    if (!first) client.print(",");
    first = false;
    client.print("{\"date\":\""); client.print(date); client.print("\"}");
  }
  client.println("]");
  f.close();
}
