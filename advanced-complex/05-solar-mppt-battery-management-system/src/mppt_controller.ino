/*
  Solar MPPT Battery Management System
  ---------------------------------------
  Perturb & Observe MPPT loop across two INA219 sensors, coulomb-counted
  SoC, SD logging, and a W5500-hosted JSON API + history endpoint.
  Board: Arduino Uno Q.
*/

#include <Wire.h>
#include <SPI.h>
#include <Adafruit_INA219.h>
#include <SD.h>
#include <Ethernet.h>

Adafruit_INA219 panelSensor(0x40);
Adafruit_INA219 battSensor(0x41);

const uint8_t PWM_PIN = 9;
const uint8_t SD_CS = 53, ETH_CS = 49;

const float BATTERY_CAPACITY_AH = 20.0;
float stateOfCharge = 80.0; // set to a known value after a full/empty calibration charge

int dutyCycle = 128; // 0-255
int dutyStep = 5;
float lastPanelPower = 0;

float panelV = 0, panelI = 0, panelW = 0, battV = 0, battI = 0;

byte mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0x02};
IPAddress ip(192, 168, 1, 181);
EthernetServer server(80);

unsigned long lastControlMs = 0, lastLogMs = 0;
const unsigned long CONTROL_INTERVAL_MS = 500;
const unsigned long LOG_INTERVAL_MS = 60000;

void setup() {
  Serial.begin(9600);
  Wire.begin();
  panelSensor.begin();
  battSensor.begin();

  pinMode(PWM_PIN, OUTPUT);
  analogWrite(PWM_PIN, dutyCycle);

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

  if (now - lastControlMs >= CONTROL_INTERVAL_MS) {
    lastControlMs = now;
    readSensors();
    perturbAndObserve();
    updateSoC(CONTROL_INTERVAL_MS / 1000.0);
  }

  if (now - lastLogMs >= LOG_INTERVAL_MS) {
    lastLogMs = now;
    logRow();
  }

  handleClient();
}

void readSensors() {
  panelV = panelSensor.getBusVoltage_V();
  panelI = panelSensor.getCurrent_mA() / 1000.0;
  panelW = panelV * panelI;

  battV = battSensor.getBusVoltage_V();
  battI = battSensor.getCurrent_mA() / 1000.0;
}

void perturbAndObserve() {
  if (panelW > lastPanelPower) {
    dutyCycle += dutyStep; // last move helped, keep going that way
  } else {
    dutyStep = -dutyStep;  // last move hurt, reverse direction
    dutyCycle += dutyStep;
  }
  dutyCycle = constrain(dutyCycle, 0, 255);
  analogWrite(PWM_PIN, dutyCycle);
  lastPanelPower = panelW;
}

void updateSoC(float dtSeconds) {
  float ampHoursDelta = (battI * dtSeconds) / 3600.0; // positive battI = charging
  stateOfCharge += (ampHoursDelta / BATTERY_CAPACITY_AH) * 100.0;
  stateOfCharge = constrain(stateOfCharge, 0, 100);
}

void ensureCsvHeader() {
  if (!SD.exists("MPPTLOG.CSV")) {
    File f = SD.open("MPPTLOG.CSV", FILE_WRITE);
    if (f) { f.println("timestamp_ms,panelV,panelI,panelW,battV,battI,soc,duty"); f.close(); }
  }
}

void logRow() {
  File f = SD.open("MPPTLOG.CSV", FILE_WRITE);
  if (!f) return;
  f.print(millis()); f.print(",");
  f.print(panelV); f.print(",");
  f.print(panelI); f.print(",");
  f.print(panelW); f.print(",");
  f.print(battV); f.print(",");
  f.print(battI); f.print(",");
  f.print(stateOfCharge); f.print(",");
  f.println(dutyCycle);
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
  client.print("{\"panelV\":"); client.print(panelV);
  client.print(",\"panelI\":"); client.print(panelI);
  client.print(",\"panelW\":"); client.print(panelW);
  client.print(",\"battV\":"); client.print(battV);
  client.print(",\"battI\":"); client.print(battI);
  client.print(",\"soc\":"); client.print(stateOfCharge);
  client.print(",\"duty\":"); client.print(dutyCycle);
  client.println("}");
}

void sendHistory(EthernetClient &client) {
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: application/json");
  client.println("Connection: close");
  client.println();

  File f = SD.open("MPPTLOG.CSV");
  if (!f) { client.println("[]"); return; }

  const uint8_t MAX_ROWS = 50;
  String lines[MAX_ROWS];
  uint8_t count = 0;
  f.readStringUntil('\n'); // skip header
  while (f.available()) {
    lines[count % MAX_ROWS] = f.readStringUntil('\n');
    count++;
  }
  f.close();

  uint8_t start = count > MAX_ROWS ? count % MAX_ROWS : 0;
  uint8_t total = min(count, MAX_ROWS);

  client.print("[");
  for (uint8_t i = 0; i < total; i++) {
    String line = lines[(start + i) % MAX_ROWS];
    int c1 = line.indexOf(',');
    int c2 = line.indexOf(',', c1 + 1);
    int c6 = line.lastIndexOf(',', line.lastIndexOf(','));
    String ts = line.substring(0, c1);
    String panelWv = line.substring(c2 + 1, line.indexOf(',', c2 + 1));
    if (i > 0) client.print(",");
    client.print("{\"t\":\""); client.print(ts); client.print("\",\"panelW\":");
    client.print(panelWv); client.print("}");
  }
  client.println("]");
}
