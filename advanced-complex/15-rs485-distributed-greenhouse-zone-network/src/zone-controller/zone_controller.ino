/*
  RS-485 Distributed Greenhouse - Zone Controller
  ------------------------------------------------------
  Runs an independent local PID climate loop regardless of bus state;
  answers hub polls and accepts setpoint pushes over RS-485.
  Board: Arduino Uno. Set MY_ZONE_ID per physical unit (1, 2, or 3).
*/

#include <SoftwareSerial.h>
#include <DHT.h>

const uint8_t MY_ZONE_ID = 1; // change to 2 or 3 for the other zone boards

const uint8_t DE_RE_PIN = 2;
const uint8_t RS485_RX = 8, RS485_TX = 9;
const uint8_t DHT_PIN = 3;
const uint8_t SOIL_PIN = A0;
const uint8_t HEATER_PIN = 4, FAN_PIN = 5, MIST_PIN = 6;

SoftwareSerial rs485(RS485_RX, RS485_TX);
DHT dht(DHT_PIN, DHT22);

float tempSetpoint = 24.0, humSetpoint = 65.0;
float tempIntegral = 0, tempLastError = 0;
float humIntegral = 0, humLastError = 0;

float curTemp = 0, curHum = 0, curSoil = 0;
bool heaterOn = false, fanOn = false, mistOn = false;

unsigned long lastControlMs = 0;
const unsigned long CONTROL_INTERVAL_MS = 2000;

void setup() {
  Serial.begin(9600);
  dht.begin();

  pinMode(HEATER_PIN, OUTPUT); pinMode(FAN_PIN, OUTPUT); pinMode(MIST_PIN, OUTPUT);
  digitalWrite(HEATER_PIN, LOW); digitalWrite(FAN_PIN, LOW); digitalWrite(MIST_PIN, LOW);

  pinMode(DE_RE_PIN, OUTPUT);
  digitalWrite(DE_RE_PIN, LOW);
  rs485.begin(9600);

  Serial.print("Zone controller "); Serial.print(MY_ZONE_ID); Serial.println(" ready.");
}

void loop() {
  unsigned long now = millis();
  if (now - lastControlMs >= CONTROL_INTERVAL_MS) {
    lastControlMs = now;
    runLocalPID();
  }

  handleBusTraffic();
}

void runLocalPID() {
  float h = dht.readHumidity();
  float t = dht.readTemperature();
  if (!isnan(h)) curHum = h;
  if (!isnan(t)) curTemp = t;
  curSoil = analogRead(SOIL_PIN);

  float dt = CONTROL_INTERVAL_MS / 1000.0;

  float tempError = tempSetpoint - curTemp;
  tempIntegral = constrain(tempIntegral + tempError * dt, -50, 50);
  float tempDeriv = (tempError - tempLastError) / dt;
  tempLastError = tempError;
  float tempOut = 8.0 * tempError + 0.05 * tempIntegral + 2.0 * tempDeriv;
  heaterOn = tempOut > 1.0;
  fanOn = tempOut < -1.0;
  digitalWrite(HEATER_PIN, heaterOn ? HIGH : LOW);
  digitalWrite(FAN_PIN, fanOn ? HIGH : LOW);

  float humError = humSetpoint - curHum;
  humIntegral = constrain(humIntegral + humError * dt, -50, 50);
  float humDeriv = (humError - humLastError) / dt;
  humLastError = humError;
  float humOut = 5.0 * humError + 0.05 * humIntegral + 1.0 * humDeriv;
  mistOn = humOut > 1.0;
  digitalWrite(MIST_PIN, mistOn ? HIGH : LOW);
}

void handleBusTraffic() {
  if (!rs485.available()) return;
  String line = rs485.readStringUntil('\n');
  line.trim();

  if (line.startsWith("POLL")) {
    int addr = line.substring(5).toInt();
    if (addr == MY_ZONE_ID) respondWithReading();
  } else if (line.startsWith("SET")) {
    int firstSp = line.indexOf(' ');
    int secondSp = line.indexOf(' ', firstSp + 1);
    int thirdSp = line.indexOf(' ', secondSp + 1);
    int addr = line.substring(firstSp + 1, secondSp).toInt();
    if (addr == MY_ZONE_ID) {
      tempSetpoint = line.substring(secondSp + 1, thirdSp).toFloat();
      humSetpoint = line.substring(thirdSp + 1).toFloat();
      sendOk();
    }
  }
}

void respondWithReading() {
  String payload = "D," + String(MY_ZONE_ID) + "," + String(curTemp, 1) + "," + String(curHum, 1) + "," +
                    String(curSoil, 0) + "," + String(heaterOn) + "," + String(fanOn) + "," + String(mistOn);
  uint8_t chk = checksum(payload);
  String frame = payload + "," + String(chk);

  digitalWrite(DE_RE_PIN, HIGH);
  delay(2);
  rs485.println(frame);
  rs485.flush();
  digitalWrite(DE_RE_PIN, LOW);
}

void sendOk() {
  digitalWrite(DE_RE_PIN, HIGH);
  delay(2);
  rs485.print("OK "); rs485.println(MY_ZONE_ID);
  rs485.flush();
  digitalWrite(DE_RE_PIN, LOW);
}

uint8_t checksum(const String &s) {
  uint16_t sum = 0;
  for (uint16_t i = 0; i < s.length(); i++) sum += (uint8_t)s[i];
  return sum % 256;
}
