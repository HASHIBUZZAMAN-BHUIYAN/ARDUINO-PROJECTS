/*
  IMU Gesture Recognition Controller
  --------------------------------------
  Timer1-driven fixed-rate IMU sampling, gesture-window detection, feature
  extraction, and an on-device nearest-neighbor classifier against
  EEPROM-stored templates. Board: Arduino Nano.
*/

#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Servo.h>
#include <EEPROM.h>

Adafruit_MPU6050 mpu;
Servo panServo, tiltServo;
const uint8_t RELAY_PIN = 6;
const uint8_t CAL_BUTTON_PIN = 2;

const uint16_t RING_SIZE = 200;
volatile float ringBuf[RING_SIZE];
volatile uint16_t ringHead = 0;
volatile bool sampleReady = false;

const float MOTION_THRESHOLD = 2.0;   // m/s^2 above gravity baseline
const uint16_t MAX_WINDOW_SAMPLES = 150;
const uint16_t SETTLE_SAMPLES = 10;

struct Features {
  float peakMag;
  uint8_t dominantAxis;
  uint16_t durationSamples;
  uint16_t zeroCrossings;
};

struct Template {
  char name[12];
  Features f;
  bool used;
};

const uint8_t MAX_TEMPLATES = 5;
Template templates[MAX_TEMPLATES];

bool calibrating = false;
char calName[12];

float windowBuf[MAX_WINDOW_SAMPLES];
uint8_t windowAxis[MAX_WINDOW_SAMPLES];
uint16_t windowLen = 0;
bool inWindow = false;
uint16_t settleCount = 0;

Features pendingFeatures;
bool hasPending = false;

void setup() {
  Serial.begin(9600);
  Wire.begin();
  if (!mpu.begin()) Serial.println("MPU6050 not found!");
  mpu.setAccelerometerRange(MPU6050_RANGE_4_G);

  panServo.attach(9);
  tiltServo.attach(10);
  panServo.write(90);
  tiltServo.write(90);

  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW);
  pinMode(CAL_BUTTON_PIN, INPUT_PULLUP);

  loadTemplates();
  setupTimer1(200); // 200 Hz sampling

  Serial.println("Ready. CAL START <name> | CAL SAVE | CAL LIST | CAL DONE");
}

void loop() {
  readSerialCal();

  if (!sampleReady) return;
  sampleReady = false;

  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);
  float mag = sqrt(a.acceleration.x * a.acceleration.x +
                    a.acceleration.y * a.acceleration.y +
                    a.acceleration.z * a.acceleration.z) - 9.8;
  uint8_t axis = dominantAxisOf(a);

  processSample(abs(mag), axis);
}

uint8_t dominantAxisOf(sensors_event_t &a) {
  float ax = abs(a.acceleration.x), ay = abs(a.acceleration.y), az = abs(a.acceleration.z);
  if (ax >= ay && ax >= az) return 0;
  if (ay >= ax && ay >= az) return 1;
  return 2;
}

void processSample(float mag, uint8_t axis) {
  if (!inWindow) {
    if (mag > MOTION_THRESHOLD) {
      inWindow = true;
      windowLen = 0;
      settleCount = 0;
    } else {
      return;
    }
  }

  if (windowLen < MAX_WINDOW_SAMPLES) {
    windowBuf[windowLen] = mag;
    windowAxis[windowLen] = axis;
    windowLen++;
  }

  if (mag < MOTION_THRESHOLD * 0.5) {
    settleCount++;
  } else {
    settleCount = 0;
  }

  if (settleCount >= SETTLE_SAMPLES || windowLen >= MAX_WINDOW_SAMPLES) {
    Features f = extractFeatures();
    inWindow = false;
    if (calibrating) {
      Serial.println("Gesture captured. Send CAL SAVE to store it.");
      pendingFeatures = f;
      hasPending = true;
    } else {
      handleRecognition(f);
    }
  }
}

Features extractFeatures() {
  Features f;
  f.peakMag = 0;
  f.durationSamples = windowLen;
  f.zeroCrossings = 0;
  uint8_t axisCounts[3] = {0, 0, 0};

  for (uint16_t i = 0; i < windowLen; i++) {
    if (windowBuf[i] > f.peakMag) f.peakMag = windowBuf[i];
    axisCounts[windowAxis[i]]++;
    if (i > 0) {
      bool wasAbove = windowBuf[i - 1] > MOTION_THRESHOLD * 0.5;
      bool isAbove = windowBuf[i] > MOTION_THRESHOLD * 0.5;
      if (wasAbove != isAbove) f.zeroCrossings++;
    }
  }
  f.dominantAxis = (axisCounts[0] >= axisCounts[1] && axisCounts[0] >= axisCounts[2]) ? 0
                   : (axisCounts[1] >= axisCounts[2]) ? 1 : 2;
  return f;
}

float featureDistance(const Features &a, const Features &b) {
  float d = 0;
  d += sq(a.peakMag - b.peakMag);
  d += sq((float)a.durationSamples - b.durationSamples) * 0.01;
  d += sq((float)a.zeroCrossings - b.zeroCrossings) * 2.0;
  d += (a.dominantAxis != b.dominantAxis) ? 10.0 : 0.0;
  return sqrt(d);
}

void handleRecognition(const Features &f) {
  float bestDist = 1e9;
  int bestIdx = -1;
  for (uint8_t i = 0; i < MAX_TEMPLATES; i++) {
    if (!templates[i].used) continue;
    float d = featureDistance(f, templates[i].f);
    if (d < bestDist) { bestDist = d; bestIdx = i; }
  }

  const float CONFIDENCE_RADIUS = 8.0;
  if (bestIdx >= 0 && bestDist < CONFIDENCE_RADIUS) {
    Serial.print("Recognized: "); Serial.println(templates[bestIdx].name);
    dispatchAction(templates[bestIdx].name);
  } else {
    Serial.println("Unrecognized gesture.");
  }
}

void dispatchAction(const char *name) {
  if (strcmp(name, "swipe-left") == 0) {
    panServo.write(45);
  } else if (strcmp(name, "swipe-right") == 0) {
    panServo.write(135);
  } else if (strcmp(name, "shake") == 0) {
    digitalWrite(RELAY_PIN, !digitalRead(RELAY_PIN));
  }
}

// ---- Timer1 fixed-rate sampling ----
void setupTimer1(int hz) {
  noInterrupts();
  TCCR1A = 0; TCCR1B = 0; TCNT1 = 0;
  long ocr = (16000000 / (8L * hz)) - 1;
  OCR1A = ocr;
  TCCR1B |= (1 << WGM12) | (1 << CS11);
  TIMSK1 |= (1 << OCIE1A);
  interrupts();
}

ISR(TIMER1_COMPA_vect) {
  sampleReady = true;
}

// ---- serial calibration protocol ----
void readSerialCal() {
  if (!Serial.available()) return;
  String line = Serial.readStringUntil('\n');
  line.trim();

  if (line.startsWith("CAL START")) {
    calibrating = true;
    hasPending = false;
    line.substring(10).toCharArray(calName, sizeof(calName));
    Serial.print("Calibrating: "); Serial.println(calName);
  } else if (line.equalsIgnoreCase("CAL SAVE")) {
    if (hasPending) {
      saveTemplate(calName, pendingFeatures);
      hasPending = false;
    } else {
      Serial.println("No captured gesture yet - perform the gesture first.");
    }
  } else if (line.equalsIgnoreCase("CAL LIST")) {
    for (uint8_t i = 0; i < MAX_TEMPLATES; i++) {
      if (templates[i].used) { Serial.print(i); Serial.print(": "); Serial.println(templates[i].name); }
    }
  } else if (line.equalsIgnoreCase("CAL DONE")) {
    calibrating = false;
    Serial.println("Calibration mode exited.");
  }
}

void saveTemplate(const char *name, const Features &f) {
  for (uint8_t i = 0; i < MAX_TEMPLATES; i++) {
    if (!templates[i].used) {
      strncpy(templates[i].name, name, sizeof(templates[i].name));
      templates[i].f = f;
      templates[i].used = true;
      EEPROM.put(i * sizeof(Template), templates[i]);
      Serial.print("Saved template slot "); Serial.println(i);
      return;
    }
  }
  Serial.println("No free template slots (max 5).");
}

void loadTemplates() {
  for (uint8_t i = 0; i < MAX_TEMPLATES; i++) {
    EEPROM.get(i * sizeof(Template), templates[i]);
    if (templates[i].name[0] == (char)0xFF || templates[i].name[0] == 0) templates[i].used = false;
  }
}
