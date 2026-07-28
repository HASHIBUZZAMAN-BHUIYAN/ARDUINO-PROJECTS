/*
  Security Fusion - Sensor Node
  -----------------------------------
  Fuses PIR motion, a door reed switch, and a piezo vibration/glass-break
  sensor into a single alert stream, sounding a local buzzer and sending
  a checksummed packet to a companion ESP32 relay over UART.
  Board: Arduino Mega 2560.
*/

const uint8_t PIR_PIN = 2;
const uint8_t REED_PIN = 3;
const uint8_t VIBRATION_PIN = A0;
const uint8_t BUZZER_PIN = 4;

const int VIBRATION_THRESHOLD = 600;

enum SensorType : uint8_t { SENSOR_PIR = 0, SENSOR_DOOR = 1, SENSOR_GLASSBREAK = 2 };

uint8_t seq = 0;

bool lastPirState = false;
bool lastDoorState = false;
bool lastVibrationState = false;

void setup() {
  Serial.begin(9600);   // USB debug
  Serial2.begin(9600);  // link to ESP32 relay

  pinMode(PIR_PIN, INPUT);
  pinMode(REED_PIN, INPUT_PULLUP);
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);

  Serial.println("Security fusion node ready.");
}

void loop() {
  checkPIR();
  checkDoor();
  checkVibration();
}

void checkPIR() {
  bool motion = digitalRead(PIR_PIN) == HIGH;
  if (motion != lastPirState) {
    lastPirState = motion;
    if (motion) triggerAlert(SENSOR_PIR, 1);
  }
}

void checkDoor() {
  bool open = digitalRead(REED_PIN) == HIGH; // pulled low when closed
  if (open != lastDoorState) {
    lastDoorState = open;
    if (open) triggerAlert(SENSOR_DOOR, 1);
  }
}

void checkVibration() {
  int reading = analogRead(VIBRATION_PIN);
  bool triggered = reading > VIBRATION_THRESHOLD;
  if (triggered && !lastVibrationState) {
    triggerAlert(SENSOR_GLASSBREAK, 1);
  }
  lastVibrationState = triggered;
}

void triggerAlert(uint8_t sensorType, uint8_t state) {
  digitalWrite(BUZZER_PIN, HIGH);
  delay(200);
  digitalWrite(BUZZER_PIN, LOW);

  sendAlert(sensorType, state);
}

void sendAlert(uint8_t sensorType, uint8_t state) {
  uint8_t checksum = sensorType ^ state ^ seq;

  Serial2.write(0xAA);       // start byte
  Serial2.write(sensorType);
  Serial2.write(state);
  Serial2.write(seq);
  Serial2.write(checksum);

  Serial.print("Alert sent: type="); Serial.print(sensorType);
  Serial.print(" state="); Serial.print(state);
  Serial.print(" seq="); Serial.println(seq);

  seq++;
}
