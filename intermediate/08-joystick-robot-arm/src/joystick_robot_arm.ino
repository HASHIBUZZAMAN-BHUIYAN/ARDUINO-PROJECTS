/*
  Joystick Robot Arm
  ---------------------
  Live joystick/button teleoperation of a 4-servo arm, with record/playback
  of waypoint sequences persisted to EEPROM. Board: Arduino Mega 2560.
*/

#include <Servo.h>
#include <EEPROM.h>

const uint8_t JOY_X_PIN = A0;   // base
const uint8_t JOY_Y_PIN = A1;   // shoulder
const uint8_t GRIPPER_BTN = 22; // joystick's own button, toggles gripper
const uint8_t ELBOW_UP_BTN = 23;
const uint8_t ELBOW_DOWN_BTN = 24;
const uint8_t RECORD_BTN = 25;
const uint8_t PLAY_BTN = 26;

const uint8_t BASE_SERVO_PIN = 9;
const uint8_t SHOULDER_SERVO_PIN = 10;
const uint8_t ELBOW_SERVO_PIN = 11;
const uint8_t GRIPPER_SERVO_PIN = 12;

const int JOY_CENTER = 512;
const int DEADZONE = 60;
const float MAX_SPEED = 1.0;
const uint8_t GRIPPER_OPEN = 30;
const uint8_t GRIPPER_CLOSED = 100;

const uint8_t MAX_WAYPOINTS = 10;
const int EEPROM_COUNT_ADDR = 0;
const int EEPROM_DATA_ADDR = 1; // 4 bytes per waypoint from here on

Servo baseServo, shoulderServo, elbowServo, gripperServo;
float baseAngle = 90, shoulderAngle = 90, elbowAngle = 90;
bool gripperClosed = false;

uint8_t waypoints[MAX_WAYPOINTS][4];
uint8_t waypointCount = 0;

bool lastGripperBtn = HIGH, lastRecordBtn = HIGH, lastPlayBtn = HIGH;

void setup() {
  baseServo.attach(BASE_SERVO_PIN);
  shoulderServo.attach(SHOULDER_SERVO_PIN);
  elbowServo.attach(ELBOW_SERVO_PIN);
  gripperServo.attach(GRIPPER_SERVO_PIN);

  pinMode(GRIPPER_BTN, INPUT_PULLUP);
  pinMode(ELBOW_UP_BTN, INPUT_PULLUP);
  pinMode(ELBOW_DOWN_BTN, INPUT_PULLUP);
  pinMode(RECORD_BTN, INPUT_PULLUP);
  pinMode(PLAY_BTN, INPUT_PULLUP);

  writeArmAngles(baseAngle, shoulderAngle, elbowAngle, GRIPPER_OPEN);
  loadWaypointsFromEeprom();

  Serial.begin(9600);
  Serial.print("Loaded waypoints: ");
  Serial.println(waypointCount);
}

void loop() {
  baseAngle = updateAxis(analogRead(JOY_X_PIN), baseAngle);
  shoulderAngle = updateAxis(analogRead(JOY_Y_PIN), shoulderAngle);

  if (digitalRead(ELBOW_UP_BTN) == LOW) elbowAngle = constrain(elbowAngle + 1.0, 10, 170);
  if (digitalRead(ELBOW_DOWN_BTN) == LOW) elbowAngle = constrain(elbowAngle - 1.0, 10, 170);

  handleGripperToggle();
  writeArmAngles(baseAngle, shoulderAngle, elbowAngle, gripperClosed ? GRIPPER_CLOSED : GRIPPER_OPEN);

  handleRecordButton();
  handlePlayButton();

  delay(15);
}

float updateAxis(int raw, float currentAngle) {
  int offset = raw - JOY_CENTER;
  if (abs(offset) < DEADZONE) return currentAngle;
  float normalized = constrain((float)offset / JOY_CENTER, -1.0, 1.0);
  return constrain(currentAngle + normalized * MAX_SPEED, 10, 170);
}

void handleGripperToggle() {
  bool state = digitalRead(GRIPPER_BTN);
  if (state == LOW && lastGripperBtn == HIGH) {
    delay(20);
    gripperClosed = !gripperClosed;
  }
  lastGripperBtn = state;
}

void handleRecordButton() {
  bool state = digitalRead(RECORD_BTN);
  if (state == LOW && lastRecordBtn == HIGH) {
    delay(20);
    recordWaypoint();
  }
  lastRecordBtn = state;
}

void handlePlayButton() {
  bool state = digitalRead(PLAY_BTN);
  if (state == LOW && lastPlayBtn == HIGH) {
    delay(20);
    playSequence();
  }
  lastPlayBtn = state;
}

void recordWaypoint() {
  if (waypointCount >= MAX_WAYPOINTS) {
    Serial.println("Waypoint memory full.");
    return;
  }
  waypoints[waypointCount][0] = (uint8_t)baseAngle;
  waypoints[waypointCount][1] = (uint8_t)shoulderAngle;
  waypoints[waypointCount][2] = (uint8_t)elbowAngle;
  waypoints[waypointCount][3] = gripperClosed ? GRIPPER_CLOSED : GRIPPER_OPEN;
  waypointCount++;
  saveWaypointsToEeprom();
  Serial.print("Recorded waypoint #");
  Serial.println(waypointCount);
}

// Smoothly interpolates from the current pose to each saved waypoint in turn.
void playSequence() {
  Serial.println("Playing sequence...");
  for (uint8_t i = 0; i < waypointCount; i++) {
    moveToPose(waypoints[i][0], waypoints[i][1], waypoints[i][2], waypoints[i][3]);
    delay(400); // pause briefly at each waypoint
  }
  Serial.println("Sequence complete.");
}

void moveToPose(uint8_t targetBase, uint8_t targetShoulder, uint8_t targetElbow, uint8_t targetGripper) {
  const uint8_t STEPS = 30;
  float startBase = baseAngle, startShoulder = shoulderAngle, startElbow = elbowAngle;

  for (uint8_t s = 1; s <= STEPS; s++) {
    float t = (float)s / STEPS;
    baseAngle = startBase + (targetBase - startBase) * t;
    shoulderAngle = startShoulder + (targetShoulder - startShoulder) * t;
    elbowAngle = startElbow + (targetElbow - startElbow) * t;
    writeArmAngles(baseAngle, shoulderAngle, elbowAngle, targetGripper);
    delay(15);
  }
  gripperClosed = (targetGripper == GRIPPER_CLOSED);
}

void writeArmAngles(float base, float shoulder, float elbow, uint8_t gripper) {
  baseServo.write((int)base);
  shoulderServo.write((int)shoulder);
  elbowServo.write((int)elbow);
  gripperServo.write(gripper);
}

void saveWaypointsToEeprom() {
  EEPROM.update(EEPROM_COUNT_ADDR, waypointCount);
  for (uint8_t i = 0; i < waypointCount; i++) {
    for (uint8_t j = 0; j < 4; j++) {
      EEPROM.update(EEPROM_DATA_ADDR + i * 4 + j, waypoints[i][j]);
    }
  }
}

void loadWaypointsFromEeprom() {
  uint8_t count = EEPROM.read(EEPROM_COUNT_ADDR);
  if (count > MAX_WAYPOINTS) count = 0; // guard against reading blank/uninitialized EEPROM
  waypointCount = count;
  for (uint8_t i = 0; i < waypointCount; i++) {
    for (uint8_t j = 0; j < 4; j++) {
      waypoints[i][j] = EEPROM.read(EEPROM_DATA_ADDR + i * 4 + j);
    }
  }
}
