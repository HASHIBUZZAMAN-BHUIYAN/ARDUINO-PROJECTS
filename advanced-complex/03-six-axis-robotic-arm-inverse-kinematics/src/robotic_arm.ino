/*
  Six-Axis Robotic Arm with Inverse Kinematics
  ---------------------------------------------
  Solves joint angles from a target (x,y,z), interpolates smoothly toward
  them, queues waypoints from serial, and checks the elbow's feedback
  potentiometer for stall/obstruction detection. Board: Arduino Uno.
*/

#include <Servo.h>
#include <EEPROM.h>

// ---- arm geometry (mm) ----
const float L1 = 105.0; // shoulder-to-elbow
const float L2 = 98.0;  // elbow-to-wrist

// ---- pins ----
const uint8_t PIN_BASE = 3, PIN_SHOULDER = 5, PIN_ELBOW = 6;
const uint8_t PIN_WRIST_P = 9, PIN_WRIST_R = 10, PIN_GRIP = 11;
const uint8_t PIN_ELBOW_FB = A0;

Servo base, shoulder, elbow, wristP, wristR, grip;

float curAngle[6] = {90, 90, 90, 90, 90, 90};   // current commanded angle per joint
float targetAngle[6] = {90, 90, 90, 90, 90, 90};
float degPerStep = 1.0; // set per-move from requested speed

const uint8_t QUEUE_SIZE = 10;
struct Cmd { bool isGrip; float x, y, z; bool open; float speed; };
Cmd queue[QUEUE_SIZE];
uint8_t qHead = 0, qTail = 0, qCount = 0;
bool executing = false;

bool obstructionFault = false;
unsigned long elbowErrorSinceMs = 0;
const float ELBOW_ERROR_THRESHOLD_DEG = 12.0;
const unsigned long ELBOW_FAULT_MS = 300;

void setup() {
  Serial.begin(9600);
  base.attach(PIN_BASE);
  shoulder.attach(PIN_SHOULDER);
  elbow.attach(PIN_ELBOW);
  wristP.attach(PIN_WRIST_P);
  wristR.attach(PIN_WRIST_R);
  grip.attach(PIN_GRIP);
  writeAllServos();
  Serial.println("Arm ready. MOVE x y z speed | GRIP open|close | SAVE n | RECALL n | RESET");
}

void loop() {
  readSerialCommands();
  if (!obstructionFault) {
    stepInterpolation();
    checkElbowFeedback();
  }
}

// ---- inverse kinematics ----
bool solveIK(float x, float y, float z, float out[6]) {
  float baseAngle = atan2(y, x) * 180.0 / PI;

  float r = sqrt(x * x + y * y);
  float reach = sqrt(r * r + z * z);
  if (reach > (L1 + L2)) reach = L1 + L2 - 0.1; // clamp to max reach

  float shoulderElevation = atan2(z, r);
  float cosElbow = (L1 * L1 + L2 * L2 - reach * reach) / (2 * L1 * L2);
  cosElbow = constrain(cosElbow, -1.0, 1.0);
  float elbowAngleRad = acos(cosElbow);

  float cosShoulder = (L1 * L1 + reach * reach - L2 * L2) / (2 * L1 * reach);
  cosShoulder = constrain(cosShoulder, -1.0, 1.0);
  float shoulderAngleRad = shoulderElevation + acos(cosShoulder);

  out[0] = baseAngle + 90;                         // base
  out[1] = shoulderAngleRad * 180.0 / PI;           // shoulder
  out[2] = 180.0 - (elbowAngleRad * 180.0 / PI);    // elbow
  out[3] = 90;                                      // wrist pitch (level, simple default)
  out[4] = 90;                                      // wrist roll (neutral)
  out[5] = curAngle[5];                              // gripper unchanged by a MOVE

  for (uint8_t i = 0; i < 6; i++) {
    if (isnan(out[i])) return false;
    out[i] = constrain(out[i], 0, 180);
  }
  return true;
}

void stepInterpolation() {
  bool allArrived = true;
  for (uint8_t i = 0; i < 6; i++) {
    float diff = targetAngle[i] - curAngle[i];
    if (abs(diff) > 0.5) {
      curAngle[i] += constrain(diff, -degPerStep, degPerStep);
      allArrived = false;
    }
  }
  writeAllServos();

  if (allArrived && executing) {
    executing = false;
    Serial.println("ok");
  }
}

void writeAllServos() {
  base.write((int)curAngle[0]);
  shoulder.write((int)curAngle[1]);
  elbow.write((int)curAngle[2]);
  wristP.write((int)curAngle[3]);
  wristR.write((int)curAngle[4]);
  grip.write((int)curAngle[5]);
}

void checkElbowFeedback() {
  int raw = analogRead(PIN_ELBOW_FB);
  float measuredAngle = map(raw, 0, 1023, 0, 180);
  float error = abs(measuredAngle - curAngle[2]);

  if (error > ELBOW_ERROR_THRESHOLD_DEG) {
    if (elbowErrorSinceMs == 0) elbowErrorSinceMs = millis();
    if (millis() - elbowErrorSinceMs > ELBOW_FAULT_MS) {
      obstructionFault = true;
      Serial.println("FAULT OBSTRUCTION");
    }
  } else {
    elbowErrorSinceMs = 0;
  }
}

// ---- serial command queue ----
void readSerialCommands() {
  if (!Serial.available()) return;
  String line = Serial.readStringUntil('\n');
  line.trim();
  if (line.length() == 0) return;

  if (line.equalsIgnoreCase("RESET")) {
    obstructionFault = false;
    elbowErrorSinceMs = 0;
    Serial.println("Fault cleared.");
    return;
  }
  if (line.startsWith("MOVE")) {
    Cmd c;
    c.isGrip = false;
    parseFloats(line, c.x, c.y, c.z, c.speed);
    enqueue(c);
  } else if (line.startsWith("GRIP")) {
    Cmd c;
    c.isGrip = true;
    c.open = line.indexOf("open") >= 0;
    enqueue(c);
  } else if (line.startsWith("SAVE")) {
    savePose(line.substring(5).toInt());
  } else if (line.startsWith("RECALL")) {
    recallPose(line.substring(7).toInt());
  }
  dispatchIfIdle();
}

void parseFloats(const String &line, float &x, float &y, float &z, float &speed) {
  int p1 = line.indexOf(' ', 5);
  x = line.substring(5, p1).toFloat();
  int p2 = line.indexOf(' ', p1 + 1);
  y = line.substring(p1 + 1, p2).toFloat();
  int p3 = line.indexOf(' ', p2 + 1);
  z = line.substring(p2 + 1, p3).toFloat();
  speed = (p3 > 0) ? line.substring(p3 + 1).toFloat() : 30.0;
}

void enqueue(const Cmd &c) {
  if (qCount >= QUEUE_SIZE) return;
  queue[qTail] = c;
  qTail = (qTail + 1) % QUEUE_SIZE;
  qCount++;
}

void dispatchIfIdle() {
  if (executing || qCount == 0 || obstructionFault) return;
  Cmd c = queue[qHead];
  qHead = (qHead + 1) % QUEUE_SIZE;
  qCount--;

  if (c.isGrip) {
    targetAngle[5] = c.open ? 60 : 150;
  } else {
    float angles[6];
    if (solveIK(c.x, c.y, c.z, angles)) {
      for (uint8_t i = 0; i < 5; i++) targetAngle[i] = angles[i];
      degPerStep = max(0.2f, c.speed / 20.0f); // deg/s -> deg/loop approximation
    }
  }
  executing = true;
}

// ---- EEPROM pose storage (8 slots x 6 floats) ----
void savePose(int slot) {
  if (slot < 0 || slot > 7) return;
  int addr = slot * 6 * sizeof(float);
  for (uint8_t i = 0; i < 6; i++) EEPROM.put(addr + i * sizeof(float), curAngle[i]);
  Serial.print("Saved pose "); Serial.println(slot);
}

void recallPose(int slot) {
  if (slot < 0 || slot > 7) return;
  int addr = slot * 6 * sizeof(float);
  for (uint8_t i = 0; i < 6; i++) EEPROM.get(addr + i * sizeof(float), targetAngle[i]);
  degPerStep = 1.0;
  executing = true;
  Serial.print("Recalling pose "); Serial.println(slot);
}
