/*
  Mega Multi-Sensor Environmental Dashboard
  ---------------------------------------------
  Reads temperature/humidity, pressure, gas, light, and sound sensors,
  displays them cycling on a 20x4 LCD, and logs a CSV row to SD once a
  minute using an RTC-derived timestamp. Board: Arduino Mega 2560.
*/

#include <DHT.h>
#include <Wire.h>
#include <Adafruit_BMP280.h>
#include <RTClib.h>
#include <SPI.h>
#include <SD.h>
#include <LiquidCrystal.h>

const uint8_t DHT_PIN = 2;
#define DHT_TYPE DHT22
DHT dht(DHT_PIN, DHT_TYPE);

Adafruit_BMP280 bmp; // I2C, shares the bus with the RTC
RTC_DS3231 rtc;

const uint8_t MQ2_PIN = A0;
const uint8_t LDR_PIN = A1;
const uint8_t SOUND_PIN = A2;
const uint8_t SD_CS_PIN = 53;

LiquidCrystal lcd(22, 23, 24, 25, 26, 27);

const char *LOG_FILENAME = "LOG.CSV";
DateTime lastLogTime;
bool sdReady = false;

uint8_t currentScreen = 0;
unsigned long lastScreenSwitch = 0;

void setup() {
  Serial.begin(9600);
  dht.begin();
  lcd.begin(20, 4);

  if (!bmp.begin(0x76)) {
    Serial.println("BMP280 not found - check wiring/address");
  }

  if (!rtc.begin()) {
    Serial.println("RTC not found - check wiring");
    while (1) delay(1000);
  }
  // First-run only: uncomment the next line, upload once to set the RTC to
  // your computer's clock at compile time, then comment it out and
  // re-upload. Otherwise the RTC resets to compile time on every reset.
  // rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));

  sdReady = SD.begin(SD_CS_PIN);
  if (!sdReady) {
    Serial.println("SD card not found - logging disabled, dashboard still works");
  } else {
    ensureLogHeader();
  }

  lastLogTime = rtc.now();
}

void loop() {
  float humidity = dht.readHumidity();
  float tempC = dht.readTemperature();
  float pressureHpa = bmp.readPressure() / 100.0F;
  int gasLevel = analogRead(MQ2_PIN);
  int lightLevel = analogRead(LDR_PIN);
  int soundLevel = analogRead(SOUND_PIN);

  updateDisplay(tempC, humidity, pressureHpa, gasLevel, lightLevel, soundLevel);

  DateTime now = rtc.now();
  if (sdReady && (now.unixtime() - lastLogTime.unixtime() >= 60)) {
    logReading(now, tempC, humidity, pressureHpa, gasLevel, lightLevel, soundLevel);
    lastLogTime = now;
  }

  delay(1000);
}

// Alternates between two screens every 4 seconds since a 20x4 LCD can't
// show all 5 readings clearly at once alongside labels.
void updateDisplay(float tempC, float humidity, float pressureHpa, int gas, int light, int sound) {
  if (millis() - lastScreenSwitch > 4000) {
    currentScreen = 1 - currentScreen;
    lastScreenSwitch = millis();
  }

  lcd.clear();
  if (currentScreen == 0) {
    lcd.setCursor(0, 0); lcd.print("Temp: "); lcd.print(tempC, 1); lcd.print((char)223); lcd.print("C");
    lcd.setCursor(0, 1); lcd.print("Humidity: "); lcd.print(humidity, 0); lcd.print("%");
    lcd.setCursor(0, 2); lcd.print("Pressure: "); lcd.print(pressureHpa, 0); lcd.print("hPa");
    lcd.setCursor(0, 3); lcd.print("Screen 1/2");
  } else {
    lcd.setCursor(0, 0); lcd.print("Gas (raw): "); lcd.print(gas);
    lcd.setCursor(0, 1); lcd.print("Light (raw): "); lcd.print(light);
    lcd.setCursor(0, 2); lcd.print("Sound (raw): "); lcd.print(sound);
    lcd.setCursor(0, 3); lcd.print("Screen 2/2");
  }
}

void ensureLogHeader() {
  if (!SD.exists(LOG_FILENAME)) {
    File f = SD.open(LOG_FILENAME, FILE_WRITE);
    if (f) {
      f.println("timestamp,temp_c,humidity_pct,pressure_hpa,gas_raw,light_raw,sound_raw");
      f.close();
    }
  }
}

void logReading(DateTime t, float tempC, float humidity, float pressureHpa, int gas, int light, int sound) {
  File f = SD.open(LOG_FILENAME, FILE_WRITE);
  if (!f) {
    Serial.println("Failed to open log file for writing");
    return;
  }

  char timestamp[20];
  snprintf(timestamp, sizeof(timestamp), "%04d-%02d-%02d %02d:%02d:%02d",
           t.year(), t.month(), t.day(), t.hour(), t.minute(), t.second());

  f.print(timestamp); f.print(",");
  f.print(tempC, 1); f.print(",");
  f.print(humidity, 0); f.print(",");
  f.print(pressureHpa, 0); f.print(",");
  f.print(gas); f.print(",");
  f.print(light); f.print(",");
  f.println(sound);

  f.close();
  Serial.println("Logged reading to SD.");
}
