/*
  DIY Weather Station
  ---------------------
  Displays temperature/humidity (DHT22) and pressure (BMP280) on a 16x2 LCD,
  with a rolling pressure trend indicator. Board: Arduino Uno.
*/

#include <DHT.h>
#include <Wire.h>
#include <Adafruit_BMP280.h>
#include <LiquidCrystal.h>

const uint8_t DHT_PIN = 2;
#define DHT_TYPE DHT22

DHT dht(DHT_PIN, DHT_TYPE);
Adafruit_BMP280 bmp; // I2C

LiquidCrystal lcd(7, 8, 9, 10, 11, 12);

// Rolling pressure history used to compute a simple trend (rising/falling/steady).
const uint8_t HISTORY_LEN = 6; // 6 samples * 2s interval = last 12 seconds... see note below
float pressureHistory[HISTORY_LEN];
uint8_t historyIndex = 0;
bool historyFilled = false;

const float TREND_THRESHOLD_HPA = 0.5; // ignore changes smaller than this (sensor noise)

void setup() {
  Serial.begin(9600);
  dht.begin();
  lcd.begin(16, 2);

  if (!bmp.begin(0x76)) { // common default address; try 0x77 if this fails on your module
    lcd.print("BMP280 not found");
    Serial.println("Could not find BMP280 - check wiring/address");
    while (1) delay(1000);
  }
}

void loop() {
  float humidity = dht.readHumidity();
  float tempC = dht.readTemperature();
  float pressureHpa = bmp.readPressure() / 100.0F;

  if (isnan(humidity) || isnan(tempC)) {
    Serial.println("DHT22 read failed, retrying next cycle");
    delay(2000);
    return;
  }

  recordPressure(pressureHpa);
  char trendChar = computeTrend();

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(tempC, 1);
  lcd.print((char)223); // degree symbol
  lcd.print("C H:");
  lcd.print(humidity, 0);
  lcd.print("%");

  lcd.setCursor(0, 1);
  lcd.print(pressureHpa, 0);
  lcd.print("hPa ");
  lcd.print(trendChar);

  Serial.print("T="); Serial.print(tempC);
  Serial.print("C H="); Serial.print(humidity);
  Serial.print("% P="); Serial.print(pressureHpa);
  Serial.print("hPa trend="); Serial.println(trendChar);

  delay(2000); // DHT22 cannot be sampled faster than ~1Hz reliably
}

void recordPressure(float p) {
  pressureHistory[historyIndex] = p;
  historyIndex = (historyIndex + 1) % HISTORY_LEN;
  if (historyIndex == 0) historyFilled = true;
}

// Compares the newest reading to the oldest one currently in the ring buffer
// to decide if pressure is rising, falling, or holding steady.
char computeTrend() {
  if (!historyFilled) return '.'; // not enough data yet

  uint8_t oldestIdx = historyIndex; // next slot to be overwritten is the oldest sample
  uint8_t newestIdx = (historyIndex + HISTORY_LEN - 1) % HISTORY_LEN;

  float delta = pressureHistory[newestIdx] - pressureHistory[oldestIdx];

  if (delta > TREND_THRESHOLD_HPA) return '^';  // rising
  if (delta < -TREND_THRESHOLD_HPA) return 'v'; // falling
  return '-';                                   // steady
}
