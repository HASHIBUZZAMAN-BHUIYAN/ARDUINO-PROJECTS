/*
  Audio Spectrum LED Matrix Visualizer
  ----------------------------------------
  Timer1-driven fixed-rate mic sampling, on-device FFT, MAX7219 bar-graph
  display, and SD-logged loud events. Board: Arduino Uno.
*/

#include <SPI.h>
#include <Wire.h>
#include <LedControl.h>
#include <RTClib.h>
#include <SD.h>
#include <arduinoFFT.h>

const uint8_t MIC_PIN = A0;
const uint8_t SENS_PIN = A1;
const uint8_t MATRIX_CS = 10, MATRIX_DIN = 11, MATRIX_CLK = 13;
const uint8_t SD_CS = 4;

LedControl lc(MATRIX_DIN, MATRIX_CLK, MATRIX_CS, 1);
RTC_DS3231 rtc;

const uint16_t SAMPLES = 128;
const double SAMPLE_RATE_HZ = 4000.0;
double vReal[SAMPLES], vImag[SAMPLES];
arduinoFFT FFT = arduinoFFT(vReal, vImag, SAMPLES, SAMPLE_RATE_HZ);

volatile int16_t ringBuf[SAMPLES];
volatile uint16_t ringHead = 0;
volatile bool bufferFull = false;

uint8_t bandHeights[8] = {0, 0, 0, 0, 0, 0, 0, 0};
uint8_t peakHold[8] = {0, 0, 0, 0, 0, 0, 0, 0};

unsigned long loudSinceMs = 0;
bool loudLogged = false;

void setup() {
  Serial.begin(9600);
  Wire.begin();

  lc.shutdown(0, false);
  lc.setIntensity(0, 8);
  lc.clearDisplay(0);

  if (!rtc.begin()) Serial.println("RTC not found!");
  if (!SD.begin(SD_CS)) Serial.println("SD init failed!");
  ensureCsvHeader();

  setupTimer1(SAMPLE_RATE_HZ);
  Serial.println("Audio visualizer running.");
}

void loop() {
  if (!bufferFull) return;
  bufferFull = false;

  for (uint16_t i = 0; i < SAMPLES; i++) {
    vReal[i] = (double)ringBuf[i];
    vImag[i] = 0;
  }

  FFT.Windowing(FFT_WIN_TYP_HAMMING, FFT_FORWARD);
  FFT.Compute(FFT_FORWARD);
  FFT.ComplexToMagnitude();

  bucketToBands();
  drawBars();
  checkLoudness();
}

void bucketToBands() {
  // Use bins 2..65 (skip DC + very low bins), grouped into 8 logical bands.
  const uint16_t startBin = 2, endBin = 64;
  const uint16_t binsPerBand = (endBin - startBin) / 8;

  for (uint8_t band = 0; band < 8; band++) {
    double sum = 0;
    uint16_t base = startBin + band * binsPerBand;
    for (uint16_t i = 0; i < binsPerBand; i++) sum += vReal[base + i];
    double avg = sum / binsPerBand;
    uint8_t height = constrain((int)(avg / 20.0), 0, 8);
    bandHeights[band] = height;
    if (height > peakHold[band]) peakHold[band] = height;
    else if (peakHold[band] > 0) peakHold[band]--;
  }
}

void drawBars() {
  for (uint8_t col = 0; col < 8; col++) {
    for (uint8_t row = 0; row < 8; row++) {
      bool lit = row < bandHeights[col] || row == peakHold[col];
      lc.setLed(0, row, col, lit);
    }
  }
}

void checkLoudness() {
  double sumSq = 0;
  for (uint16_t i = 0; i < SAMPLES; i++) sumSq += vReal[i] * vReal[i];
  double rms = sqrt(sumSq / SAMPLES);

  int threshold = map(analogRead(SENS_PIN), 0, 1023, 50, 800);

  if (rms > threshold) {
    if (loudSinceMs == 0) loudSinceMs = millis();
    if (!loudLogged && millis() - loudSinceMs > 1000) {
      logEvent(rms, threshold);
      loudLogged = true;
    }
  } else {
    loudSinceMs = 0;
    loudLogged = false;
  }
}

void ensureCsvHeader() {
  if (!SD.exists("NOISELOG.CSV")) {
    File f = SD.open("NOISELOG.CSV", FILE_WRITE);
    if (f) { f.println("timestamp,rms,threshold"); f.close(); }
  }
}

void logEvent(double rms, int threshold) {
  File f = SD.open("NOISELOG.CSV", FILE_WRITE);
  if (!f) return;
  DateTime now = rtc.now();
  f.print(now.timestamp(DateTime::TIMESTAMP_FULL)); f.print(",");
  f.print(rms); f.print(",");
  f.println(threshold);
  f.close();
  Serial.println("Loud event logged.");
}

// ---- Timer1 fixed-rate sampling ----
void setupTimer1(double hz) {
  noInterrupts();
  TCCR1A = 0; TCCR1B = 0; TCNT1 = 0;
  long ocr = (16000000 / (8 * hz)) - 1;
  OCR1A = ocr;
  TCCR1B |= (1 << WGM12) | (1 << CS11);
  TIMSK1 |= (1 << OCIE1A);
  interrupts();
}

ISR(TIMER1_COMPA_vect) {
  int16_t sample = analogRead(MIC_PIN) - 512; // center around 0
  ringBuf[ringHead] = sample;
  ringHead++;
  if (ringHead >= SAMPLES) {
    ringHead = 0;
    bufferFull = true;
  }
}
