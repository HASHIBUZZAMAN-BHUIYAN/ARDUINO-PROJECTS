/*
  Warehouse AGV Dispatch - Dispatch Console
  ------------------------------------------------
  Keypad job entry, LCD queue display, and a checksummed UART protocol
  to the AGV with ACK/retry flow control so only one job is ever in
  flight at a time. Board: Arduino Uno.
*/

#include <Keypad.h>
#include <LiquidCrystal.h>
#include <SoftwareSerial.h>

const byte ROWS = 4, COLS = 4;
char keys[ROWS][COLS] = {
  {'1','2','3','A'},
  {'4','5','6','B'},
  {'7','8','9','C'},
  {'*','0','#','D'}
};
byte rowPins[ROWS] = {2, 3, 4, 5};
byte colPins[COLS] = {6, 7, 8, 9};
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

LiquidCrystal lcd(10, 11, 12, 13, A0, A1);
SoftwareSerial link(A2, A3); // RX, TX - crossed to Mega Serial2

#pragma pack(push, 1)
struct JobPacket {
  uint8_t stx = 0x02;
  uint8_t len = 3;
  uint8_t cmd = 1; // JOB
  uint8_t jobId;
  uint8_t destinationStation;
  uint8_t crc8;
  uint8_t etx = 0x03;
};
#pragma pack(pop)

const uint8_t QUEUE_SIZE = 8;
uint8_t queueStations[QUEUE_SIZE];
uint8_t qHead = 0, qTail = 0, qCount = 0;

uint8_t nextJobId = 1;
bool jobInFlight = false;
uint8_t inFlightJobId = 0;
uint8_t inFlightStation = 0;
unsigned long jobSentAtMs = 0;
uint8_t retriesLeft = 0;
const unsigned long ACK_TIMEOUT_MS = 1000;

String entryBuffer = "";

void setup() {
  Serial.begin(9600);
  link.begin(115200);
  lcd.begin(16, 2);
  lcd.print("AGV Dispatch");
  lcd.setCursor(0, 1);
  lcd.print("Enter station #");
}

void loop() {
  handleKeypad();
  handleLinkTraffic();
  handleRetryTimeout();
  dispatchIfIdle();
}

void handleKeypad() {
  char key = keypad.getKey();
  if (!key) return;

  if (key >= '0' && key <= '9') {
    entryBuffer += key;
    lcd.setCursor(0, 1);
    lcd.print("Station: " + entryBuffer + "   ");
  } else if (key == '#') {
    if (entryBuffer.length() > 0) {
      enqueueJob(entryBuffer.toInt());
      entryBuffer = "";
    }
  } else if (key == '*') {
    entryBuffer = "";
    lcd.setCursor(0, 1);
    lcd.print("Cleared         ");
  }
}

void enqueueJob(uint8_t station) {
  if (qCount >= QUEUE_SIZE) return;
  queueStations[qTail] = station;
  qTail = (qTail + 1) % QUEUE_SIZE;
  qCount++;
  lcd.setCursor(0, 1);
  lcd.print("Queued: " + String(station) + "      ");
}

void dispatchIfIdle() {
  if (jobInFlight || qCount == 0) return;

  uint8_t station = queueStations[qHead];
  qHead = (qHead + 1) % QUEUE_SIZE;
  qCount--;

  sendJob(nextJobId, station);
  inFlightJobId = nextJobId;
  inFlightStation = station;
  nextJobId++;
  jobInFlight = true;
  jobSentAtMs = millis();
  retriesLeft = 2;
}

void sendJob(uint8_t jobId, uint8_t station) {
  JobPacket pkt;
  pkt.jobId = jobId;
  pkt.destinationStation = station;
  uint8_t body[3] = {pkt.cmd, pkt.jobId, pkt.destinationStation};
  pkt.crc8 = crc8(body, 3);

  link.write(pkt.stx);
  link.write(pkt.len);
  link.write(pkt.cmd);
  link.write(pkt.jobId);
  link.write(pkt.destinationStation);
  link.write(pkt.crc8);
  link.write(pkt.etx);

  lcd.clear();
  lcd.print("Job " + String(jobId) + " -> St " + String(station));
}

void handleLinkTraffic() {
  if (!link.available()) return;
  String line = link.readStringUntil('\n');
  line.trim();

  if (line.startsWith("ACK,")) {
    uint8_t jobId = line.substring(4).toInt();
    if (jobId == inFlightJobId) {
      lcd.setCursor(0, 1);
      lcd.print("Executing...     ");
    }
  } else if (line.startsWith("DONE,")) {
    uint8_t jobId = line.substring(5).toInt();
    if (jobId == inFlightJobId) {
      jobInFlight = false;
      lcd.setCursor(0, 1);
      lcd.print("Job " + String(jobId) + " DONE   ");
    }
  }
}

void handleRetryTimeout() {
  if (!jobInFlight) return;
  if (millis() - jobSentAtMs < ACK_TIMEOUT_MS) return;

  if (retriesLeft > 0) {
    retriesLeft--;
    sendJob(inFlightJobId, inFlightStation);
    jobSentAtMs = millis();
  } else {
    jobInFlight = false;
    lcd.setCursor(0, 1);
    lcd.print("Dispatch FAILED  ");
  }
}

uint8_t crc8(const uint8_t *data, size_t len) {
  uint8_t crc = 0;
  for (size_t i = 0; i < len; i++) {
    crc ^= data[i];
    for (uint8_t b = 0; b < 8; b++) crc = (crc & 0x80) ? (crc << 1) ^ 0x07 : (crc << 1);
  }
  return crc;
}
