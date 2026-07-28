/*
  Autonomous Maze-Solving Robot
  ---------------------------------
  Fuses IR wall sensors, ultrasonic, encoders, and IMU heading into a
  grid pose; explores via flood-fill, stores the solved maze + optimal
  path to EEPROM, and replays it at speed on the next run.
  Board: Arduino Mega 2560.
*/

#include <Wire.h>
#include <EEPROM.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>

const uint8_t IR_FL = A0, IR_FR = A1, IR_SIDE = A2;
const uint8_t US_TRIG = 6, US_ECHO = 7;
const uint8_t ENC_L_A = 2, ENC_L_B = 3;
const uint8_t ENC_R_A = 18, ENC_R_B = 19;
const uint8_t MOTOR_L_FWD = 8, MOTOR_L_REV = 9;
const uint8_t MOTOR_R_FWD = 10, MOTOR_R_REV = 11;

const int IR_WALL_THRESHOLD = 400;
const long TICKS_PER_CELL = 960; // calibrate for your wheel/encoder/cell size

const uint8_t MAZE_SIZE = 6; // 6x6 grid
const uint8_t GOAL_X = 5, GOAL_Y = 5;

Adafruit_MPU6050 mpu;

volatile long ticksL = 0, ticksR = 0;

// bit 0=N,1=E,2=S,3=W wall present
uint8_t walls[MAZE_SIZE][MAZE_SIZE];
uint8_t floodDist[MAZE_SIZE][MAZE_SIZE];

int8_t robotX = 0, robotY = 0;
uint8_t heading = 0; // 0=N,1=E,2=S,3=W
float imuYaw = 0;

bool speedMode = false;
uint8_t storedPath[64];
uint8_t storedPathLen = 0;
uint8_t pathIndex = 0;

const int EEPROM_MAGIC_ADDR = 0;
const int EEPROM_MAGIC_VALUE = 0xA5;
const int EEPROM_WALLS_ADDR = 4;
const int EEPROM_PATH_LEN_ADDR = EEPROM_WALLS_ADDR + (MAZE_SIZE * MAZE_SIZE);
const int EEPROM_PATH_ADDR = EEPROM_PATH_LEN_ADDR + 1;

void encLA_ISR() { digitalRead(ENC_L_B) ? ticksL++ : ticksL--; }
void encRA_ISR() { digitalRead(ENC_R_B) ? ticksR++ : ticksR--; }

void setup() {
  Serial.begin(9600);
  Wire.begin();
  if (!mpu.begin()) Serial.println("MPU6050 not found!");

  pinMode(IR_FL, INPUT); pinMode(IR_FR, INPUT); pinMode(IR_SIDE, INPUT);
  pinMode(US_TRIG, OUTPUT); pinMode(US_ECHO, INPUT);
  pinMode(ENC_L_A, INPUT_PULLUP); pinMode(ENC_L_B, INPUT_PULLUP);
  pinMode(ENC_R_A, INPUT_PULLUP); pinMode(ENC_R_B, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(ENC_L_A), encLA_ISR, RISING);
  attachInterrupt(digitalPinToInterrupt(ENC_R_A), encRA_ISR, RISING);

  pinMode(MOTOR_L_FWD, OUTPUT); pinMode(MOTOR_L_REV, OUTPUT);
  pinMode(MOTOR_R_FWD, OUTPUT); pinMode(MOTOR_R_REV, OUTPUT);

  initFloodDistances();

  if (EEPROM.read(EEPROM_MAGIC_ADDR) == EEPROM_MAGIC_VALUE) {
    loadMazeFromEEPROM();
    speedMode = true;
    Serial.println("Stored maze found - running in SPEED mode.");
  } else {
    Serial.println("No stored maze - running in EXPLORE mode.");
  }
}

void loop() {
  updateImuYaw();

  if (speedMode) {
    runSpeedMode();
  } else {
    runExploreMode();
  }
}

// ---- exploration: flood-fill ----
void runExploreMode() {
  senseWallsAtCurrentCell();
  floodFill();

  if (robotX == GOAL_X && robotY == GOAL_Y) {
    Serial.println("Goal reached. Maze solved, path stored.");
    uint8_t path[64];
    uint8_t len = computeOptimalPath(path);
    saveMazeToEEPROM(path, len);
    stopMotors();
    while (true) delay(1000); // halt; power-cycle to run speed mode
  }

  int8_t bestDx = 0, bestDy = 0;
  uint8_t bestDir = findBestNeighborDirection();
  driveOneCell(bestDir);
}

void senseWallsAtCurrentCell() {
  int fl = analogRead(IR_FL);
  int fr = analogRead(IR_FR);
  int side = analogRead(IR_SIDE);
  long frontCm = readUltrasonicCm();

  bool frontWall = (fl > IR_WALL_THRESHOLD || fr > IR_WALL_THRESHOLD || frontCm < 12);
  bool sideWall = side > IR_WALL_THRESHOLD;

  uint8_t frontDir = heading;
  uint8_t sideDir = (heading + 1) % 4; // right-hand side sensor

  if (frontWall) markWall(robotX, robotY, frontDir);
  if (sideWall) markWall(robotX, robotY, sideDir);
}

void markWall(int8_t x, int8_t y, uint8_t dir) {
  walls[y][x] |= (1 << dir);
  int8_t nx = x, ny = y;
  moveCoord(nx, ny, dir);
  if (nx >= 0 && nx < MAZE_SIZE && ny >= 0 && ny < MAZE_SIZE) {
    walls[ny][nx] |= (1 << ((dir + 2) % 4));
  }
}

void moveCoord(int8_t &x, int8_t &y, uint8_t dir) {
  if (dir == 0) y++;
  else if (dir == 1) x++;
  else if (dir == 2) y--;
  else if (dir == 3) x--;
}

void initFloodDistances() {
  for (uint8_t y = 0; y < MAZE_SIZE; y++)
    for (uint8_t x = 0; x < MAZE_SIZE; x++)
      floodDist[y][x] = abs(GOAL_X - x) + abs(GOAL_Y - y);
}

void floodFill() {
  bool changed = true;
  while (changed) {
    changed = false;
    for (uint8_t y = 0; y < MAZE_SIZE; y++) {
      for (uint8_t x = 0; x < MAZE_SIZE; x++) {
        if (x == GOAL_X && y == GOAL_Y) continue;
        uint8_t best = 255;
        for (uint8_t dir = 0; dir < 4; dir++) {
          if (walls[y][x] & (1 << dir)) continue; // wall blocks this direction
          int8_t nx = x, ny = y;
          moveCoord(nx, ny, dir);
          if (nx < 0 || nx >= MAZE_SIZE || ny < 0 || ny >= MAZE_SIZE) continue;
          if (floodDist[ny][nx] < best) best = floodDist[ny][nx];
        }
        if (best != 255 && best + 1 < floodDist[y][x]) {
          floodDist[y][x] = best + 1;
          changed = true;
        }
      }
    }
  }
}

uint8_t findBestNeighborDirection() {
  uint8_t bestDir = heading;
  uint8_t bestDist = 255;
  for (uint8_t dir = 0; dir < 4; dir++) {
    if (walls[robotY][robotX] & (1 << dir)) continue;
    int8_t nx = robotX, ny = robotY;
    moveCoord(nx, ny, dir);
    if (nx < 0 || nx >= MAZE_SIZE || ny < 0 || ny >= MAZE_SIZE) continue;
    if (floodDist[ny][nx] < bestDist) { bestDist = floodDist[ny][nx]; bestDir = dir; }
  }
  return bestDir;
}

uint8_t computeOptimalPath(uint8_t path[]) {
  uint8_t len = 0;
  int8_t x = 0, y = 0;
  while (!(x == GOAL_X && y == GOAL_Y) && len < 64) {
    uint8_t bestDir = 0, bestDist = 255;
    for (uint8_t dir = 0; dir < 4; dir++) {
      if (walls[y][x] & (1 << dir)) continue;
      int8_t nx = x, ny = y;
      moveCoord(nx, ny, dir);
      if (nx < 0 || nx >= MAZE_SIZE || ny < 0 || ny >= MAZE_SIZE) continue;
      if (floodDist[ny][nx] < bestDist) { bestDist = floodDist[ny][nx]; bestDir = dir; }
    }
    path[len++] = bestDir;
    moveCoord(x, y, bestDir);
  }
  return len;
}

// ---- speed run: replay stored path ----
void runSpeedMode() {
  if (pathIndex >= storedPathLen) { stopMotors(); return; }
  driveOneCell(storedPath[pathIndex]);
  pathIndex++;
}

// ---- motion primitives ----
void driveOneCell(uint8_t targetDir) {
  if (targetDir != heading) turnToHeading(targetDir);
  driveForwardOneCell();
  heading = targetDir;
  moveCoord(robotX, robotY, targetDir);
}

void turnToHeading(uint8_t targetDir) {
  int8_t diff = ((int8_t)targetDir - (int8_t)heading + 4) % 4;
  float targetYawDelta = diff * 90.0;
  float startYaw = imuYaw;
  bool turningRight = diff == 1 || diff == -3;

  while (abs(normalizeAngle(imuYaw - startYaw)) < abs(targetYawDelta) - 3) {
    updateImuYaw();
    if (turningRight || diff == 2) { setMotors(120, -120); } else { setMotors(-120, 120); }
  }
  stopMotors();
}

void driveForwardOneCell() {
  ticksL = 0; ticksR = 0;
  float kp = 4.0;
  while ((ticksL + ticksR) / 2 < TICKS_PER_CELL) {
    // closed-loop straight-line correction using encoder differential
    int error = ticksL - ticksR;
    int base = 150;
    setMotors(base - kp * error, base + kp * error);
  }
  stopMotors();
}

void setMotors(int leftSpeed, int rightSpeed) {
  leftSpeed = constrain(leftSpeed, -255, 255);
  rightSpeed = constrain(rightSpeed, -255, 255);
  analogWrite(MOTOR_L_FWD, leftSpeed > 0 ? leftSpeed : 0);
  analogWrite(MOTOR_L_REV, leftSpeed < 0 ? -leftSpeed : 0);
  analogWrite(MOTOR_R_FWD, rightSpeed > 0 ? rightSpeed : 0);
  analogWrite(MOTOR_R_REV, rightSpeed < 0 ? -rightSpeed : 0);
}

void stopMotors() { setMotors(0, 0); }

float normalizeAngle(float a) {
  while (a > 180) a -= 360;
  while (a < -180) a += 360;
  return a;
}

void updateImuYaw() {
  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);
  static unsigned long lastMs = 0;
  unsigned long now = millis();
  float dt = (lastMs == 0) ? 0 : (now - lastMs) / 1000.0;
  lastMs = now;
  imuYaw += g.gyro.z * (180.0 / PI) * dt;
}

long readUltrasonicCm() {
  digitalWrite(US_TRIG, LOW); delayMicroseconds(2);
  digitalWrite(US_TRIG, HIGH); delayMicroseconds(10);
  digitalWrite(US_TRIG, LOW);
  long duration = pulseIn(US_ECHO, HIGH, 20000);
  return duration / 58;
}

// ---- EEPROM persistence ----
void saveMazeToEEPROM(uint8_t path[], uint8_t len) {
  EEPROM.write(EEPROM_MAGIC_ADDR, EEPROM_MAGIC_VALUE);
  int addr = EEPROM_WALLS_ADDR;
  for (uint8_t y = 0; y < MAZE_SIZE; y++)
    for (uint8_t x = 0; x < MAZE_SIZE; x++)
      EEPROM.write(addr++, walls[y][x]);
  EEPROM.write(EEPROM_PATH_LEN_ADDR, len);
  for (uint8_t i = 0; i < len; i++) EEPROM.write(EEPROM_PATH_ADDR + i, path[i]);
}

void loadMazeFromEEPROM() {
  int addr = EEPROM_WALLS_ADDR;
  for (uint8_t y = 0; y < MAZE_SIZE; y++)
    for (uint8_t x = 0; x < MAZE_SIZE; x++)
      walls[y][x] = EEPROM.read(addr++);
  storedPathLen = EEPROM.read(EEPROM_PATH_LEN_ADDR);
  for (uint8_t i = 0; i < storedPathLen; i++) storedPath[i] = EEPROM.read(EEPROM_PATH_ADDR + i);
}
