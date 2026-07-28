/*
  Vibration FFT Predictive Maintenance Monitor
  -----------------------------------------------
  Timer1-driven fixed-rate accelerometer sampling, on-device FFT, baseline
  learning, and closed-loop machine isolation on sustained anomaly.
  Board: Arduino Uno.
*/

#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <RTClib.h>
#include <arduinoFFT.h>

const uint8_t ADXL345_ADDR = 0x53;
const uint8_t RELAY_PIN = 4;
const uint8_t BUZZER_PIN = 5;
const uint8_t RESET_BTN_PIN = 2;
const uint8_t SD_CS = 10;

const uint16_t SAMPLES = 256;          // must be a power of 2
const double SAMPLE_RATE_HZ = 800.0;

double vReal[SAMPLES];
double vImag[SAMPLES];
arduinoFFT FFT = arduinoFFT(vReal, vImag, SAMPLES, SAMPLE_RATE_HZ);

volatile int16_t ringBuf[SAMPLES];
volatile uint16_t ringHead = 0;
volatile bool bufferFull = false;

RTC_DS3231 rtc;

// baseline state
const uint8_t LEARNING_WINDOWS = 60;
uint8_t windowCount = 0;
double baselineFreq = 0, baselineRMS = 0;
bool learned = false;

uint8_t anomalyStreak = 0;
const uint8_t ANOMALY_TRIP_THRESHOLD = 5;
bool faulted = false;

void setup() {
  Serial.begin(9600);
  Wire.begin();
  initADXL345();

  pinMode(RELAY_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(RESET_BTN_PIN, INPUT_PULLUP);
  digitalWrite(RELAY_PIN, HIGH); // energized = machine allowed to run
  digitalWrite(BUZZER_PIN, LOW);

  if (!rtc.begin()) Serial.println("RTC not found!");
  if (!SD.begin(SD_CS)) Serial.println("SD init failed!");
  ensureCsvHeader();

  setupTimer1(SAMPLE_RATE_HZ);
  Serial.println("Learning baseline vibration signature...");
}

void loop() {
  if (digitalRead(RESET_BTN_PIN) == LOW && faulted) {
    faulted = false;
    anomalyStreak = 0;
    digitalWrite(RELAY_PIN, HIGH);
    digitalWrite(BUZZER_PIN, LOW);
    Serial.println("Fault cleared by operator.");
    delay(300);
  }

  if (!bufferFull) return;
  bufferFull = false;

  for (uint16_t i = 0; i < SAMPLES; i++) {
    vReal[i] = (double)ringBuf[i];
    vImag[i] = 0;
  }

  FFT.Windowing(FFT_WIN_TYP_HAMMING, FFT_FORWARD);
  FFT.Compute(FFT_FORWARD);
  FFT.ComplexToMagnitude();
  double dominantFreq = FFT.MajorPeak();
  double rms = computeRMS();

  if (!learned) {
    baselineFreq += dominantFreq;
    baselineRMS += rms;
    windowCount++;
    Serial.println("LEARNING");
    if (windowCount >= LEARNING_WINDOWS) {
      baselineFreq /= LEARNING_WINDOWS;
      baselineRMS /= LEARNING_WINDOWS;
      learned = true;
      Serial.print("Baseline learned: freq="); Serial.print(baselineFreq);
      Serial.print("Hz rms="); Serial.println(baselineRMS);
    }
  } else {
    bool anomaly = (abs(dominantFreq - baselineFreq) > baselineFreq * 0.25) ||
                   (rms > baselineRMS * 1.8);
    if (anomaly) {
      anomalyStreak++;
      Serial.println("ANOMALY");
    } else {
      anomalyStreak = 0;
      Serial.println("OK");
    }

    if (anomalyStreak >= ANOMALY_TRIP_THRESHOLD && !faulted) {
      faulted = true;
      digitalWrite(RELAY_PIN, LOW); // isolate machine
      digitalWrite(BUZZER_PIN, HIGH);
      Serial.println("FAULT: machine isolated, sustained vibration anomaly.");
    }
  }

  logWindow(dominantFreq, rms, faulted);
}

double computeRMS() {
  double sumSq = 0;
  for (uint16_t i = 0; i < SAMPLES; i++) sumSq += vReal[i] * vReal[i];
  return sqrt(sumSq / SAMPLES);
}

// ---- Timer1 fixed-rate sampling ISR ----
void setupTimer1(double hz) {
  noInterrupts();
  TCCR1A = 0;
  TCCR1B = 0;
  TCNT1 = 0;
  long ocr = (16000000 / (8 * hz)) - 1;
  OCR1A = ocr;
  TCCR1B |= (1 << WGM12);  // CTC mode
  TCCR1B |= (1 << CS11);   // prescaler 8
  TIMSK1 |= (1 << OCIE1A);
  interrupts();
}

ISR(TIMER1_COMPA_vect) {
  int16_t mag = readAccelMagnitude();
  ringBuf[ringHead] = mag;
  ringHead++;
  if (ringHead >= SAMPLES) {
    ringHead = 0;
    bufferFull = true;
  }
}

int16_t readAccelMagnitude() {
  Wire.beginTransmission(ADXL345_ADDR);
  Wire.write(0x32); // DATAX0
  Wire.endTransmission(false);
  Wire.requestFrom(ADXL345_ADDR, (uint8_t)6);
  int16_t x = Wire.read() | (Wire.read() << 8);
  int16_t y = Wire.read() | (Wire.read() << 8);
  int16_t z = Wire.read() | (Wire.read() << 8);
  return (int16_t)sqrt((long)x * x + (long)y * y + (long)z * z);
}

void initADXL345() {
  Wire.beginTransmission(ADXL345_ADDR);
  Wire.write(0x2D); // POWER_CTL
  Wire.write(0x08); // measurement mode
  Wire.endTransmission();
}

void ensureCsvHeader() {
  if (!SD.exists("VIBLOG.CSV")) {
    File f = SD.open("VIBLOG.CSV", FILE_WRITE);
    if (f) { f.println("timestamp,dominantHz,rms,fault"); f.close(); }
  }
}

void logWindow(double freq, double rms, bool fault) {
  File f = SD.open("VIBLOG.CSV", FILE_WRITE);
  if (!f) return;
  DateTime now = rtc.now();
  f.print(now.timestamp(DateTime::TIMESTAMP_FULL)); f.print(",");
  f.print(freq); f.print(",");
  f.print(rms); f.print(",");
  f.println(fault);
  f.close();
}
