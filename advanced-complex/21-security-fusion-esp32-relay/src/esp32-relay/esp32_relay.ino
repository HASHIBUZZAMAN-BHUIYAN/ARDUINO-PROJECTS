/*
  Security Fusion - ESP32 WiFi Relay
  -----------------------------------------
  Receives checksummed alert packets from the Mega sensor-fusion node
  over UART and relays them to an MQTT broker over WiFi.
  Board: ESP32 DevKit (any common "ESP32 Dev Module" variant).
*/

#include <WiFi.h>
#include <PubSubClient.h>

// ---- credentials placeholder — replace before use, never commit real secrets ----
const char* WIFI_SSID = "YOUR_WIFI_SSID";
const char* WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";
const char* MQTT_BROKER = "YOUR_MQTT_BROKER_IP_OR_HOST";
const int   MQTT_PORT = 1883;
const char* MQTT_TOPIC = "security/alerts";
// ---------------------------------------------------------------------------------

WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

const uint8_t START_BYTE = 0xAA;

void setup() {
  Serial.begin(115200);          // USB debug
  Serial2.begin(9600, SERIAL_8N1, 16, 17); // RX=16, TX=17, link to Mega

  connectWiFi();
  mqttClient.setServer(MQTT_BROKER, MQTT_PORT);

  Serial.println("ESP32 relay ready.");
}

void loop() {
  if (!mqttClient.connected()) reconnectMqtt();
  mqttClient.loop();

  readPacketIfAny();
}

void connectWiFi() {
  Serial.print("Connecting to WiFi");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.print("\nWiFi connected, IP: ");
  Serial.println(WiFi.localIP());
}

void reconnectMqtt() {
  while (!mqttClient.connected()) {
    Serial.print("Connecting to MQTT broker...");
    String clientId = "esp32-security-relay-" + String(random(0xffff), HEX);
    if (mqttClient.connect(clientId.c_str())) {
      Serial.println("connected.");
    } else {
      Serial.print("failed, rc="); Serial.print(mqttClient.state());
      Serial.println(" retrying in 2s");
      delay(2000);
    }
  }
}

void readPacketIfAny() {
  if (Serial2.available() < 5) return;

  if (Serial2.peek() != START_BYTE) {
    Serial2.read(); // resync: discard stray byte
    return;
  }

  uint8_t buf[5];
  Serial2.readBytes(buf, 5);

  uint8_t sensorType = buf[1], state = buf[2], seq = buf[3], checksum = buf[4];
  uint8_t computed = sensorType ^ state ^ seq;

  if (computed != checksum) {
    Serial.println("Checksum mismatch, dropping packet.");
    return;
  }

  publishAlert(sensorType, state, seq);
}

void publishAlert(uint8_t sensorType, uint8_t state, uint8_t seq) {
  const char *name = sensorNameFor(sensorType);

  char payload[96];
  snprintf(payload, sizeof(payload), "{\"sensor\":\"%s\",\"state\":%d,\"seq\":%d}", name, state, seq);

  if (mqttClient.connected()) {
    mqttClient.publish(MQTT_TOPIC, payload);
    Serial.print("Published: "); Serial.println(payload);
  } else {
    Serial.println("MQTT not connected, alert dropped (see Known Limitations - no store-and-forward).");
  }
}

const char *sensorNameFor(uint8_t sensorType) {
  switch (sensorType) {
    case 0: return "PIR";
    case 1: return "Door";
    case 2: return "Glassbreak";
    default: return "Unknown";
  }
}
