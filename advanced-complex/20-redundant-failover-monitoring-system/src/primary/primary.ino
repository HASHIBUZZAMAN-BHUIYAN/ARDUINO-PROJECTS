/*
  Redundant Failover Monitoring - Primary Unit
  ---------------------------------------------------
  Active by default: drives shared sensors/actuators and transmits a
  heartbeat. Yields (goes listen-only) if it detects a heartbeat it did
  not itself send, meaning Standby has already promoted itself.
  Board: Arduino Uno.
*/

#include <SoftwareSerial.h>
#include <OneWire.h>
#include <DallasTemperature.h>

const uint8_t ONEWIRE_PIN = 2;
const uint8_t GAS_PIN = A0;
const uint8_t LEAK_PIN = 3;
const uint8_t SIREN_PIN = 4, VALVE_PIN = 5;
const uint8_t LINK_TX = 6, LINK_RX = 7;

OneWire oneWire(ONEWIRE_PIN);
DallasTemperature tempSensor(&oneWire);
SoftwareSerial link(LINK_RX, LINK_TX);

const char *MY_ID = "PRIMARY";
bool isActive = true; // Primary starts active by default

uint16_t txSeq = 0;
unsigned long lastHeartbeatSentMs = 0;
unsigned long lastHeartbeatHeardMs = 0;
const unsigned long HEARTBEAT_INTERVAL_MS = 250;
const unsigned long SILENCE_TIMEOUT_MS = 1000;

void setup() {
  Serial.begin(9600);
  link.begin(9600);
  tempSensor.begin();

  pinMode(LEAK_PIN, INPUT_PULLUP);
  setActuatorPinMode(isActive);

  lastHeartbeatHeardMs = millis();
  Serial.println("Primary starting ACTIVE.");
}

void loop() {
  readIncomingHeartbeats();

  if (isActive) {
    runActiveLogic();
  } else {
    checkForSilenceAndPromote(); // Primary can re-promote if it hasn't heard Standby in a while
  }
}

void setActuatorPinMode(bool active) {
  pinMode(SIREN_PIN, active ? OUTPUT : INPUT);
  pinMode(VALVE_PIN, active ? OUTPUT : INPUT);
  if (!active) {
    // ensure no pull-up sneaks a weak drive onto the shared line
    digitalWrite(SIREN_PIN, LOW);
    digitalWrite(VALVE_PIN, LOW);
  }
}

void runActiveLogic() {
  float temp = readTempC();
  int gas = analogRead(GAS_PIN);
  bool leak = digitalRead(LEAK_PIN) == LOW;

  bool alarm = (temp > 60.0) || (gas > 600) || leak;
  digitalWrite(SIREN_PIN, alarm ? HIGH : LOW);
  digitalWrite(VALVE_PIN, leak ? LOW : HIGH); // close valve (LOW) on leak

  sendHeartbeatIfDue(alarm);
}

void sendHeartbeatIfDue(bool alarmState) {
  unsigned long now = millis();
  if (now - lastHeartbeatSentMs < HEARTBEAT_INTERVAL_MS) return;
  lastHeartbeatSentMs = now;
  txSeq++;
  link.print("HB,"); link.print(txSeq); link.print(","); link.println(alarmState ? "ALARM" : "OK");
  Serial.print("ACTIVE hb seq="); Serial.println(txSeq);
}

void readIncomingHeartbeats() {
  if (!link.available()) return;
  String line = link.readStringUntil('\n');
  line.trim();
  if (!line.startsWith("HB,")) return;

  lastHeartbeatHeardMs = millis();

  // If we are active and hear ANY heartbeat, it must be from Standby
  // (since we are the one generating our own) - that means Standby has
  // promoted, and split-brain avoidance requires we yield immediately.
  if (isActive) {
    isActive = false;
    setActuatorPinMode(false);
    Serial.println("YIELDING (standby is active) - Primary now listen-only.");
  }
}

void checkForSilenceAndPromote() {
  if (millis() - lastHeartbeatHeardMs > SILENCE_TIMEOUT_MS) {
    isActive = true;
    setActuatorPinMode(true);
    Serial.println("Primary RE-PROMOTED (standby went silent).");
  }
}

float readTempC() {
  tempSensor.requestTemperatures();
  return tempSensor.getTempCByIndex(0);
}
