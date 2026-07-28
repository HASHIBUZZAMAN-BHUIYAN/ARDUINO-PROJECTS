/*
  GPS Data Logging Tracker
  ----------------------------
  Parses NMEA data from a GPS module and logs a timestamped CSV track to SD,
  using an RTC as a backup clock before a GPS fix is acquired. Board: Arduino Uno.
*/

#include <SoftwareSerial.h>
#include <TinyGPSPlus.h>
#include <SPI.h>
#include <SD.h>
#include <Wire.h>
#include <RTClib.h>

const uint8_t GPS_RX_PIN = 4; // Arduino RX <- GPS TX
const uint8_t GPS_TX_PIN = 3; // Arduino TX -> GPS RX
const uint8_t SD_CS_PIN = 10;
const uint8_t STATUS_LED_PIN = 6;

SoftwareSerial gpsSerial(GPS_RX_PIN, GPS_TX_PIN);
TinyGPSPlus gps;
RTC_DS3231 rtc;

const char *LOG_FILENAME = "TRACK.CSV";
bool sdReady = false;
bool rtcSyncedFromGps = false;
unsigned long lastLogTime = 0;

void setup() {
  Serial.begin(9600);
  gpsSerial.begin(9600); // NEO-6M default baud
  pinMode(STATUS_LED_PIN, OUTPUT);

  if (!rtc.begin()) {
    Serial.println("RTC not found - check wiring");
  }

  sdReady = SD.begin(SD_CS_PIN);
  if (!sdReady) {
    Serial.println("SD card not found - check wiring/formatting");
  } else {
    ensureLogHeader();
  }

  Serial.println("GPS tracker starting - waiting for satellite fix...");
}

void loop() {
  // Feed every available byte from the GPS module into the parser;
  // TinyGPSPlus assembles complete NMEA sentences internally.
  while (gpsSerial.available()) {
    gps.encode(gpsSerial.read());
  }

  bool hasFix = gps.location.isValid() && gps.location.age() < 2000;
  digitalWrite(STATUS_LED_PIN, hasFix ? HIGH : (millis() / 300) % 2); // blink while searching

  if (hasFix && !rtcSyncedFromGps && gps.date.isValid() && gps.time.isValid()) {
    syncRtcFromGps();
  }

  if (sdReady && millis() - lastLogTime >= 1000) {
    logRow(hasFix);
    lastLogTime = millis();
  }
}

void syncRtcFromGps() {
  rtc.adjust(DateTime(gps.date.year(), gps.date.month(), gps.date.day(),
                       gps.time.hour(), gps.time.minute(), gps.time.second()));
  rtcSyncedFromGps = true;
  Serial.println("RTC synced from GPS time.");
}

void ensureLogHeader() {
  if (!SD.exists(LOG_FILENAME)) {
    File f = SD.open(LOG_FILENAME, FILE_WRITE);
    if (f) {
      f.println("timestamp,lat,lng,speed_kmph,altitude_m");
      f.close();
    }
  }
}

void logRow(bool hasFix) {
  File f = SD.open(LOG_FILENAME, FILE_WRITE);
  if (!f) return;

  DateTime now = rtc.now(); // always available, even before a GPS fix
  char timestamp[20];
  snprintf(timestamp, sizeof(timestamp), "%04d-%02d-%02d %02d:%02d:%02d",
           now.year(), now.month(), now.day(), now.hour(), now.minute(), now.second());

  f.print(timestamp);
  f.print(",");

  if (hasFix) {
    f.print(gps.location.lat(), 6); f.print(",");
    f.print(gps.location.lng(), 6); f.print(",");
    f.print(gps.speed.kmph(), 1); f.print(",");
    f.println(gps.altitude.meters(), 1);
  } else {
    f.println("NO_FIX,NO_FIX,0,0");
  }

  f.close();
}
