/*
  Dual-Board Wireless Security System - Remote Sensor Node
  --------------------------------------------------------------
  Watches a PIR sensor and a door reed switch, transmitting event packets
  (and periodic heartbeats) to the base station over nRF24L01.
  Board: Arduino Nano. Flash this sketch to the REMOTE board.
*/

#include <SPI.h>
#include <RF24.h>

const uint8_t CE_PIN = 9;
const uint8_t CSN_PIN = 10;
const uint8_t PIR_PIN = 2;
const uint8_t REED_PIN = A0;

RF24 radio(CE_PIN, CSN_PIN);
const byte PIPE_ADDRESS[6] = "ALRM1"; // must match the base station exactly

enum SensorType : uint8_t { SENSOR_PIR = 0, SENSOR_DOOR = 1, SENSOR_HEARTBEAT = 2 };

struct Packet {
  uint8_t sensorType;
  uint8_t state;    // 1 = active/open, 0 = clear/closed (ignored for heartbeat)
  uint16_t seq;
};

uint16_t sequenceNumber = 0;
bool lastPirState = false;
bool lastReedState = false;
unsigned long lastHeartbeat = 0;
const unsigned long HEARTBEAT_INTERVAL_MS = 10000;

void setup() {
  pinMode(PIR_PIN, INPUT);
  pinMode(REED_PIN, INPUT);

  Serial.begin(9600);
  radio.begin();
  radio.setPALevel(RF24_PA_LOW); // LOW is plenty for typical in-home range and saves power
  radio.openWritingPipe(PIPE_ADDRESS);
  radio.stopListening(); // this node only ever transmits
}

void loop() {
  bool pirState = digitalRead(PIR_PIN);
  bool reedState = digitalRead(REED_PIN);

  if (pirState != lastPirState) {
    sendPacket(SENSOR_PIR, pirState);
    lastPirState = pirState;
  }

  if (reedState != lastReedState) {
    sendPacket(SENSOR_DOOR, reedState);
    lastReedState = reedState;
  }

  if (millis() - lastHeartbeat > HEARTBEAT_INTERVAL_MS) {
    sendPacket(SENSOR_HEARTBEAT, 0);
    lastHeartbeat = millis();
  }

  delay(100); // light debounce on the digital reads
}

void sendPacket(uint8_t sensorType, uint8_t state) {
  Packet pkt = {sensorType, state, sequenceNumber++};
  bool ok = radio.write(&pkt, sizeof(pkt));
  Serial.print("Sent type="); Serial.print(sensorType);
  Serial.print(" state="); Serial.print(state);
  Serial.println(ok ? " (ack ok)" : " (no ack!)");
}
