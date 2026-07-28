/*
  Dual-Board Wireless Security System - Base Station
  --------------------------------------------------------
  Listens for sensor event / heartbeat packets from the remote node over
  nRF24L01, shows status on an LCD, and sounds a siren on alerts.
  Board: Arduino Uno. Flash this sketch to the BASE board.
*/

#include <SPI.h>
#include <RF24.h>
#include <LiquidCrystal.h>

const uint8_t CE_PIN = 9;
const uint8_t CSN_PIN = 10;
const uint8_t BUZZER_PIN = 8;

RF24 radio(CE_PIN, CSN_PIN);
const byte PIPE_ADDRESS[6] = "ALRM1"; // must match the remote node exactly

LiquidCrystal lcd(2, 3, 4, 5, 6, 7);

enum SensorType : uint8_t { SENSOR_PIR = 0, SENSOR_DOOR = 1, SENSOR_HEARTBEAT = 2 };

struct Packet {
  uint8_t sensorType;
  uint8_t state;
  uint16_t seq;
};

unsigned long lastPacketTime = 0;
const unsigned long OFFLINE_THRESHOLD_MS = 25000; // > 2x remote heartbeat interval
bool offlineWarningShown = false;
bool sounding = false;

void setup() {
  Serial.begin(9600);
  lcd.begin(16, 2);
  pinMode(BUZZER_PIN, OUTPUT);

  radio.begin();
  radio.setPALevel(RF24_PA_LOW);
  radio.openReadingPipe(1, PIPE_ADDRESS);
  radio.startListening();

  lastPacketTime = millis();
  showStatus("System ready", "Listening...");
}

void loop() {
  if (radio.available()) {
    Packet pkt;
    radio.read(&pkt, sizeof(pkt));
    lastPacketTime = millis();
    offlineWarningShown = false;
    handlePacket(pkt);
  }

  if (!offlineWarningShown && (millis() - lastPacketTime > OFFLINE_THRESHOLD_MS)) {
    offlineWarningShown = true;
    showStatus("WARNING:", "REMOTE OFFLINE");
    Serial.println("Remote node offline - no packets received recently.");
  }
}

void handlePacket(const Packet &pkt) {
  switch (pkt.sensorType) {
    case SENSOR_PIR:
      Serial.println(pkt.state ? "PIR: motion detected" : "PIR: clear");
      if (pkt.state) triggerAlert("Zone: Motion");
      else silence();
      break;

    case SENSOR_DOOR:
      Serial.println(pkt.state ? "Door: opened" : "Door: closed");
      if (pkt.state) triggerAlert("Zone: Door");
      else silence();
      break;

    case SENSOR_HEARTBEAT:
      Serial.println("Heartbeat received - remote node online.");
      if (!sounding) showStatus("System ready", "Listening...");
      break;
  }
}

void triggerAlert(const char *zone) {
  sounding = true;
  showStatus("ALERT!", zone);
  tone(BUZZER_PIN, 2500);
}

void silence() {
  sounding = false;
  noTone(BUZZER_PIN);
  showStatus("System ready", "Listening...");
}

void showStatus(const char *line1, const char *line2) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(line1);
  lcd.setCursor(0, 1);
  lcd.print(line2);
}
