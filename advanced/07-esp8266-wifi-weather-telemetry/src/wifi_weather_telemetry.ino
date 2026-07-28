/*
  ESP8266 WiFi Weather Telemetry
  ------------------------------------
  Reads DHT22 + BMP280 and pushes readings to a local HTTP server over WiFi,
  using an ESP8266 (ESP-01) as an AT-command WiFi co-processor.
  Board: Arduino Uno Q.

  No real credentials are included below - fill in your own network and
  server details before use.
*/

#include <WiFiEspAT.h>
#include <DHT.h>
#include <Wire.h>
#include <Adafruit_BMP280.h>

// --- Fill these in for your own network/server; do not commit real values ---
const char WIFI_SSID[] = "YOUR_WIFI_SSID";
const char WIFI_PASSWORD[] = "YOUR_WIFI_PASSWORD";
const char SERVER_HOST[] = "192.168.1.50"; // your local logging server
const uint16_t SERVER_PORT = 8080;
// -----------------------------------------------------------------------------

const uint8_t DHT_PIN = 2;
#define DHT_TYPE DHT22
DHT dht(DHT_PIN, DHT_TYPE);
Adafruit_BMP280 bmp;

WiFiClient client;
const unsigned long SEND_INTERVAL_MS = 60000;
unsigned long lastSendTime = 0;

void setup() {
  Serial.begin(9600);
  Serial1.begin(9600); // UART to the ESP8266; use whichever hardware/software serial port is wired to D8/D9

  dht.begin();
  if (!bmp.begin(0x76)) {
    Serial.println("BMP280 not found - check wiring/address");
  }

  WiFi.init(Serial1);
  if (WiFi.status() == WL_NO_MODULE) {
    Serial.println("ESP8266 module not responding - check wiring/baud rate");
    while (1) delay(1000);
  }

  connectToWiFi();
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    connectToWiFi();
  }

  if (millis() - lastSendTime >= SEND_INTERVAL_MS) {
    sendReading();
    lastSendTime = millis();
  }
}

void connectToWiFi() {
  Serial.print("Connecting to WiFi: ");
  Serial.println(WIFI_SSID);

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  uint8_t attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println();
    Serial.print("Connected, IP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println();
    Serial.println("WiFi connection failed - will retry next cycle.");
  }
}

void sendReading() {
  float humidity = dht.readHumidity();
  float tempC = dht.readTemperature();
  float pressureHpa = bmp.readPressure() / 100.0F;

  if (isnan(humidity) || isnan(tempC)) {
    Serial.println("DHT22 read failed, skipping this cycle.");
    return;
  }

  Serial.print("Sending: T="); Serial.print(tempC);
  Serial.print(" H="); Serial.print(humidity);
  Serial.print(" P="); Serial.println(pressureHpa);

  if (!client.connect(SERVER_HOST, SERVER_PORT)) {
    Serial.println("Connection to server failed.");
    return;
  }

  // Simple GET request with readings as query parameters -- a minimal
  // logging endpoint just needs to read these off the request line.
  client.print("GET /log?t=");
  client.print(tempC, 1);
  client.print("&h=");
  client.print(humidity, 0);
  client.print("&p=");
  client.print(pressureHpa, 0);
  client.println(" HTTP/1.1");
  client.print("Host: ");
  client.println(SERVER_HOST);
  client.println("Connection: close");
  client.println();

  // Read just the HTTP status line back for a basic success/failure check.
  unsigned long start = millis();
  while (client.connected() && !client.available() && millis() - start < 5000) {
    delay(10);
  }
  if (client.available()) {
    String statusLine = client.readStringUntil('\n');
    Serial.print("Server response: ");
    Serial.println(statusLine);
  }

  client.stop();
}
