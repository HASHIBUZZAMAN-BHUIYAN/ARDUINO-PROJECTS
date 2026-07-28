/*
  PID Greenhouse Climate Controller
  ---------------------------------
  Two independent PID loops (temperature -> heater/fan, humidity -> mister)
  plus an RTC-scheduled grow-light relay. Logs to SD once a minute and
  serves a JSON API + setpoint endpoint over a W5500 Ethernet shield.
  Board: Arduino Mega 2560.
*/

#include <Wire.h>
#include <SPI.h>
#include <DHT.h>
#include <BH1750.h>
#include <RTClib.h>
#include <SD.h>
#include <Ethernet.h>

// ---- pins ----
const uint8_t DHT_PIN = 2;
const uint8_t SOIL_PIN = A0;
const uint8_t HEATER_PIN = 3, FAN_PIN = 4, MIST_PIN = 5, LIGHT_PIN = 6;
const uint8_t SD_CS = 53, ETH_CS = 49;
const bool RELAY_ACTIVE_LOW = true;

DHT dht(DHT_PIN, DHT22);
BH1750 lightMeter;
RTC_DS3231 rtc;

byte mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0x01};
IPAddress ip(192, 168, 1, 180);
EthernetServer server(80);

// ---- PID state ----
struct PID {
  float kp, ki, kd;
  float setpoint;
  float integral = 0, lastError = 0;
};

PID tempPID = {8.0, 0.05, 2.0, 24.0};
PID humPID  = {5.0, 0.05, 1.0, 65.0};

float curTemp = 0, curHum = 0, curLight = 0, curSoil = 0;
bool heaterOn = false, fanOn = false, mistOn = false, lightOn = false;

unsigned long lastControlMs = 0, lastLogMs = 0;
const unsigned long CONTROL_INTERVAL_MS = 2000;
const unsigned long LOG_INTERVAL_MS = 60000;

const int LIGHT_ON_HOUR = 6, LIGHT_OFF_HOUR = 20;

void setup() {
  Serial.begin(9600);
  dht.begin();
  Wire.begin();
  lightMeter.begin();

  if (!rtc.begin()) Serial.println("RTC not found!");
  // First upload only: uncomment to set the RTC to your computer's clock.
  // rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));

  pinMode(HEATER_PIN, OUTPUT);
  pinMode(FAN_PIN, OUTPUT);
  pinMode(MIST_PIN, OUTPUT);
  pinMode(LIGHT_PIN, OUTPUT);
  setRelay(HEATER_PIN, false);
  setRelay(FAN_PIN, false);
  setRelay(MIST_PIN, false);
  setRelay(LIGHT_PIN, false);

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
    runPID();
    updateLightSchedule();
  }

  if (now - lastLogMs >= LOG_INTERVAL_MS) {
    lastLogMs = now;
    logRow();
  }

  handleClient();
}

void readSensors() {
  float h = dht.readHumidity();
  float t = dht.readTemperature();
  if (!isnan(h)) curHum = h;
  if (!isnan(t)) curTemp = t;
  curLight = lightMeter.readLightLevel();
  curSoil = analogRead(SOIL_PIN);
}

float pidStep(PID &p, float measured, float dtSeconds) {
  float error = p.setpoint - measured;
  p.integral += error * dtSeconds;
  p.integral = constrain(p.integral, -50, 50); // anti-windup clamp
  float derivative = (error - p.lastError) / dtSeconds;
  p.lastError = error;
  return p.kp * error + p.ki * p.integral + p.kd * derivative;
}

void runPID() {
  float dt = CONTROL_INTERVAL_MS / 1000.0;

  float tempOut = pidStep(tempPID, curTemp, dt);
  heaterOn = tempOut > 1.0;   // too cold -> heater
  fanOn = tempOut < -1.0;     // too hot -> fan
  setRelay(HEATER_PIN, heaterOn);
  setRelay(FAN_PIN, fanOn);

  float humOut = pidStep(humPID, curHum, dt);
  mistOn = humOut > 1.0;      // too dry -> mister
  setRelay(MIST_PIN, mistOn);
}

void updateLightSchedule() {
  DateTime now = rtc.now();
  bool shouldBeOn = now.hour() >= LIGHT_ON_HOUR && now.hour() < LIGHT_OFF_HOUR;
  if (shouldBeOn != lightOn) {
    lightOn = shouldBeOn;
    setRelay(LIGHT_PIN, lightOn);
  }
}

void setRelay(uint8_t pin, bool on) {
  digitalWrite(pin, (on != RELAY_ACTIVE_LOW) ? HIGH : LOW);
}

void ensureCsvHeader() {
  if (!SD.exists("GREENHOUSE.CSV")) {
    File f = SD.open("GREENHOUSE.CSV", FILE_WRITE);
    if (f) {
      f.println("timestamp,temp,hum,light,soil,heater,fan,mist,lightRelay");
      f.close();
    }
  }
}

void logRow() {
  File f = SD.open("GREENHOUSE.CSV", FILE_WRITE);
  if (!f) return;
  DateTime now = rtc.now();
  f.print(now.timestamp(DateTime::TIMESTAMP_FULL)); f.print(",");
  f.print(curTemp); f.print(",");
  f.print(curHum); f.print(",");
  f.print(curLight); f.print(",");
  f.print(curSoil); f.print(",");
  f.print(heaterOn); f.print(",");
  f.print(fanOn); f.print(",");
  f.print(mistOn); f.print(",");
  f.println(lightOn);
  f.close();
}

void handleClient() {
  EthernetClient client = server.available();
  if (!client) return;

  String reqLine = client.readStringUntil('\r');
  client.flush();

  if (reqLine.indexOf("GET /api/data") >= 0) {
    sendJson(client);
  } else if (reqLine.indexOf("GET /setpoint") >= 0) {
    applySetpoint(reqLine);
    sendJson(client);
  } else {
    client.println("HTTP/1.1 404 Not Found");
    client.println("Connection: close");
    client.println();
  }
  delay(1);
  client.stop();
}

void applySetpoint(const String &reqLine) {
  int tIdx = reqLine.indexOf("t=");
  int hIdx = reqLine.indexOf("h=");
  if (tIdx >= 0) tempPID.setpoint = reqLine.substring(tIdx + 2).toFloat();
  if (hIdx >= 0) humPID.setpoint = reqLine.substring(hIdx + 2).toFloat();
}

void sendJson(EthernetClient &client) {
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: application/json");
  client.println("Connection: close");
  client.println();
  client.print("{\"temp\":"); client.print(curTemp);
  client.print(",\"hum\":"); client.print(curHum);
  client.print(",\"light\":"); client.print(curLight);
  client.print(",\"soil\":"); client.print(curSoil);
  client.print(",\"heater\":"); client.print(heaterOn ? 1 : 0);
  client.print(",\"fan\":"); client.print(fanOn ? 1 : 0);
  client.print(",\"mist\":"); client.print(mistOn ? 1 : 0);
  client.print(",\"light_relay\":"); client.print(lightOn ? 1 : 0);
  client.print(",\"setpointTemp\":"); client.print(tempPID.setpoint);
  client.print(",\"setpointHum\":"); client.print(humPID.setpoint);
  client.println("}");
}
