/*
  Ethernet Home Automation Server
  ------------------------------------
  Hosts a tiny web page over a wired Ethernet shield to toggle 4 relay
  channels from any browser on the local network. Board: Arduino Uno Q.

  NOTE: no authentication is implemented - treat this as trusted-LAN-only,
  not something to expose to the public internet.
*/

#include <SPI.h>
#include <Ethernet.h>

// Any locally-administered MAC works for a private LAN.
byte mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};
// Pick an address on your LAN's subnet, outside your router's DHCP range.
IPAddress ip(192, 168, 1, 177);

EthernetServer server(80);

const uint8_t RELAY_PINS[4] = {2, 3, 4, 5};
const char *DEVICE_NAMES[4] = {"Lamp", "Fan", "Heater", "Outlet"};
bool relayState[4] = {false, false, false, false};

void setup() {
  Serial.begin(9600);

  for (uint8_t i = 0; i < 4; i++) {
    pinMode(RELAY_PINS[i], OUTPUT);
    digitalWrite(RELAY_PINS[i], LOW);
  }

  Ethernet.init(10); // most W5100/W5500 shields use pin 10 for CS
  Ethernet.begin(mac, ip);
  server.begin();

  Serial.print("Server ready at http://");
  Serial.println(Ethernet.localIP());
}

void loop() {
  EthernetClient client = server.available();
  if (!client) return;

  String requestLine = client.readStringUntil('\r');
  client.flush();

  int toggleIndex = parseToggleIndex(requestLine);
  if (toggleIndex >= 0 && toggleIndex < 4) {
    relayState[toggleIndex] = !relayState[toggleIndex];
    digitalWrite(RELAY_PINS[toggleIndex], relayState[toggleIndex] ? HIGH : LOW);
    Serial.print(DEVICE_NAMES[toggleIndex]);
    Serial.println(relayState[toggleIndex] ? " -> ON" : " -> OFF");
  }

  sendPage(client);
  delay(1); // give the client time to receive the response before closing
  client.stop();
}

// Looks for "GET /toggle?d=N" in the request's first line and extracts N.
int parseToggleIndex(const String &requestLine) {
  int paramIdx = requestLine.indexOf("d=");
  if (requestLine.indexOf("GET /toggle") == -1 || paramIdx == -1) return -1;
  return requestLine.substring(paramIdx + 2, paramIdx + 3).toInt();
}

void sendPage(EthernetClient &client) {
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: text/html");
  client.println("Connection: close");
  client.println();

  client.println("<!DOCTYPE html><html><head><title>Home Automation Hub</title></head><body>");
  client.println("<h1>Home Automation Hub</h1><ul>");

  for (uint8_t i = 0; i < 4; i++) {
    client.print("<li>");
    client.print(DEVICE_NAMES[i]);
    client.print(": <b>");
    client.print(relayState[i] ? "ON" : "OFF");
    client.print("</b> - <a href=\"/toggle?d=");
    client.print(i);
    client.println("\">toggle</a></li>");
  }

  client.println("</ul></body></html>");
}
