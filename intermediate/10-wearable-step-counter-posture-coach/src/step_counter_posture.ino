/*
  Wearable Step Counter & Posture Coach
  -----------------------------------------
  Counts steps from accelerometer magnitude crossings and buzzes a motor
  after a sustained forward-slouch angle. Board: Arduino Nano.
*/

#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Wire.h>

Adafruit_MPU6050 mpu;

const uint8_t MOTOR_PIN = 9;

// Step detection thresholds, in units of g (Earth gravity). Tune these to
// your mounting location -- a wrist swings much more than a belt.
const float STEP_THRESHOLD_HIGH = 1.3;
const float STEP_THRESHOLD_LOW = 0.8;
bool armedForStep = false; // true once magnitude has dipped below the low threshold
unsigned long lastStepTime = 0;
const unsigned long MIN_STEP_INTERVAL_MS = 250; // reject unrealistically fast "steps"

unsigned long stepCount = 0;

// Posture thresholds.
const float SLOUCH_ANGLE_DEG = 35.0;
const unsigned long SLOUCH_HOLD_MS = 4000; // must slouch continuously this long to trigger
unsigned long slouchStartTime = 0;
bool currentlySlouching = false;

void setup() {
  Serial.begin(9600);
  pinMode(MOTOR_PIN, OUTPUT);

  if (!mpu.begin()) {
    Serial.println("MPU6050 not found - check wiring/address");
    while (1) delay(1000);
  }
  mpu.setAccelerometerRange(MPU6050_RANGE_4_G);
  Serial.println("Step counter + posture coach ready.");
}

void loop() {
  sensors_event_t accel, gyro, temp;
  mpu.getEvent(&accel, &gyro, &temp);

  checkStep(accel.acceleration.x, accel.acceleration.y, accel.acceleration.z);
  checkPosture(accel.acceleration.x, accel.acceleration.z);

  delay(50); // ~20Hz sampling is plenty for step/posture detection
}

// Uses a two-threshold (hysteresis) crossing on total acceleration magnitude
// to count one step per footfall while ignoring small jitter.
void checkStep(float ax, float ay, float az) {
  float magnitudeG = sqrt(ax * ax + ay * ay + az * az) / 9.81;

  if (magnitudeG < STEP_THRESHOLD_LOW) {
    armedForStep = true; // we've seen the "airborne" dip, ready to count the next spike
  }

  if (armedForStep && magnitudeG > STEP_THRESHOLD_HIGH) {
    unsigned long now = millis();
    if (now - lastStepTime > MIN_STEP_INTERVAL_MS) {
      stepCount++;
      lastStepTime = now;
      Serial.print("Steps: ");
      Serial.println(stepCount);
    }
    armedForStep = false;
  }
}

// Estimates forward lean from the accelerometer's static tilt and buzzes
// only if the slouch persists continuously past SLOUCH_HOLD_MS.
void checkPosture(float ax, float az) {
  float leanAngleDeg = atan2(ax, az) * 180.0 / PI;

  bool slouchingNow = abs(leanAngleDeg) > SLOUCH_ANGLE_DEG;

  if (slouchingNow && !currentlySlouching) {
    currentlySlouching = true;
    slouchStartTime = millis();
  } else if (!slouchingNow) {
    currentlySlouching = false;
  }

  if (currentlySlouching && (millis() - slouchStartTime > SLOUCH_HOLD_MS)) {
    buzzReminder();
    slouchStartTime = millis(); // avoid re-buzzing every loop; wait another full hold period
  }
}

void buzzReminder() {
  for (uint8_t i = 0; i < 3; i++) {
    digitalWrite(MOTOR_PIN, HIGH);
    delay(150);
    digitalWrite(MOTOR_PIN, LOW);
    delay(150);
  }
}
