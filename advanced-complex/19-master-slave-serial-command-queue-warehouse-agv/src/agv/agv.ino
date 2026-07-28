/*
  Warehouse AGV Dispatch - AGV
  ----------------------------------
  Receives one job at a time over a checksummed UART protocol, then
  line-follows with closed-loop PID steering until it detects the
  destination station's marker. Board: Arduino Mega 2560.
*/

const uint8_t IR_PINS[5] = {A8, A9, A10, A11, A12};
const uint8_t MOTOR_PINS[4] = {8, 9, 10, 11};

const int LINE_THRESHOLD = 500;

bool hasJob = false;
uint8_t currentJobId = 0;
uint8_t destinationStation = 0;
uint8_t stationsPassedSinceJobStart = 0;

float integral = 0, lastError = 0;

void setup() {
  Serial.begin(9600);   // USB debug
  Serial2.begin(115200); // link to dispatch console

  for (uint8_t i = 0; i < 5; i++) pinMode(IR_PINS[i], INPUT);
  for (uint8_t i = 0; i < 4; i++) pinMode(MOTOR_PINS[i], OUTPUT);
  stopMotors();

  Serial.println("AGV ready, idle.");
}

void loop() {
  readPacketIfAny();

  if (hasJob) {
    lineFollowStep();
    if (detectedStationMarker()) {
      stopMotors();
      hasJob = false;
      Serial2.print("DONE,"); Serial2.println(currentJobId);
      Serial.print("Arrived, job "); Serial.println(currentJobId);
    }
  }
}

void readPacketIfAny() {
  if (Serial2.available() < 7) return; // full packet size

  if (Serial2.peek() != 0x02) { Serial2.read(); return; } // resync on stray byte

  uint8_t buf[7];
  Serial2.readBytes(buf, 7);

  uint8_t stx = buf[0], len = buf[1], cmd = buf[2], jobId = buf[3], station = buf[4], crc = buf[5], etx = buf[6];
  if (stx != 0x02 || etx != 0x03) return;

  uint8_t body[3] = {cmd, jobId, station};
  if (crc8(body, 3) != crc) {
    Serial.println("CRC8 mismatch, dropping job packet.");
    return;
  }

  if (cmd == 1) { // JOB
    currentJobId = jobId;
    destinationStation = station;
    hasJob = true;
    integral = 0; lastError = 0;
    Serial2.print("ACK,"); Serial2.println(jobId);
    Serial.print("Job "); Serial.print(jobId); Serial.print(" accepted -> station "); Serial.println(station);
  }
}

void lineFollowStep() {
  int readings[5];
  for (uint8_t i = 0; i < 5; i++) readings[i] = analogRead(IR_PINS[i]);

  // weighted position error: negative = line to the left, positive = to the right
  float weights[5] = {-2, -1, 0, 1, 2};
  float weightedSum = 0, total = 0;
  for (uint8_t i = 0; i < 5; i++) {
    float onLine = readings[i] > LINE_THRESHOLD ? 1.0 : 0.0;
    weightedSum += weights[i] * onLine;
    total += onLine;
  }
  float error = (total > 0) ? (weightedSum / total) : lastError; // hold last error if line lost momentarily

  float dt = 0.02;
  integral = constrain(integral + error * dt, -10, 10);
  float derivative = (error - lastError) / dt;
  lastError = error;

  float correction = 40.0 * error + 1.0 * integral + 5.0 * derivative;
  int baseSpeed = 150;
  int leftSpeed = constrain(baseSpeed + correction, 0, 255);
  int rightSpeed = constrain(baseSpeed - correction, 0, 255);

  analogWrite(MOTOR_PINS[0], leftSpeed);
  digitalWrite(MOTOR_PINS[1], LOW);
  analogWrite(MOTOR_PINS[2], rightSpeed);
  digitalWrite(MOTOR_PINS[3], LOW);
}

bool detectedStationMarker() {
  // a station marker is a wide perpendicular cross-line: all 5 sensors see line at once.
  bool allDark = true;
  for (uint8_t i = 0; i < 5; i++) {
    if (analogRead(IR_PINS[i]) < LINE_THRESHOLD) { allDark = false; break; }
  }
  if (allDark) {
    stationsPassedSinceJobStart++;
    return stationsPassedSinceJobStart >= destinationStation;
  }
  return false;
}

void stopMotors() {
  for (uint8_t i = 0; i < 4; i++) analogWrite(MOTOR_PINS[i], 0);
}

uint8_t crc8(const uint8_t *data, size_t len) {
  uint8_t crc = 0;
  for (size_t i = 0; i < len; i++) {
    crc ^= data[i];
    for (uint8_t b = 0; b < 8; b++) crc = (crc & 0x80) ? (crc << 1) ^ 0x07 : (crc << 1);
  }
  return crc;
}
