# Security Fusion with ESP32 WiFi Relay

A Mega fuses PIR motion, a door reed switch, and a piezo glass-break/vibration sensor into a single security event stream, and sends any trigger over UART as a checksummed packet to a companion ESP32 DevKit, which relays the alert over WiFi to an MQTT broker (and/or a webhook) — bridging this repo's Arduino-family sensor fusion with a WiFi-capable companion device.

## Board(s) & Roles

- **Arduino Mega 2560 ("Sensor Fusion")** — reads PIR motion, a door reed switch, and a piezo vibration/glass-break sensor; on any trigger, packages a checksummed alert packet and sends it over UART.
- **ESP32 DevKit ("WiFi Relay")** — a companion device, *not* normally part of this repo's Arduino-family board pool, included here specifically as the lead project's paired relay. Receives alert packets over UART, connects to WiFi, and publishes each alert to an MQTT topic (and optionally a webhook) using `PubSubClient`.

This is a cross-repo flagship: a fuller companion writeup for the ESP32 relay side also exists in the **ESP32-PROJECTS** repository's root README.

## Components

| Part | Qty |
|---|---|
| Arduino Mega 2560 | 1 |
| ESP32 DevKit | 1 |
| HC-SR501 PIR motion sensor | 1 |
| Magnetic reed switch (door) | 1 |
| Piezo vibration/glass-break sensor (analog, with comparator threshold) | 1 |
| Buzzer (local siren) | 1 |
| Jumper wires | ~15 |

## Architecture

The Mega continuously polls all 3 sensors; on any trigger (motion, door open, or a glass-break vibration spike above threshold) it immediately builds a small fixed-format packet — sensor type, state, sequence number, and an XOR checksum — and transmits it once over UART. The ESP32 listens on its UART RX pin, validates the checksum, and on a valid alert connects to WiFi (if not already connected) and publishes a JSON payload to an MQTT topic via `PubSubClient`, with a webhook POST as a documented alternative. See [architecture.md](architecture.md) for the full diagram and data-flow walkthrough.

## Wiring

### Sensor Fusion (Mega)

| Arduino Mega Pin | Component | Notes |
|---|---|---|
| D2 | PIR OUT | |
| D3 | Door reed switch | `INPUT_PULLUP` |
| A0 | Piezo vibration sensor (analog) | threshold-compared in software |
| D4 | Local siren buzzer | |
| D16 (TX2), D17 (RX2) | UART to ESP32 (`Serial2`) | |

### WiFi Relay (ESP32 DevKit)

| ESP32 Pin | Component | Notes |
|---|---|---|
| GPIO16 (RX2), GPIO17 (TX2) | UART to Mega (`Serial2`) | crossed: Mega TX2->ESP32 RX2, Mega RX2->ESP32 TX2 |

See [wiring/wiring.md](wiring/wiring.md) for the full ASCII diagram.

## Networking & Protocol

**UART (Mega → ESP32):** 9600 baud, a fixed 5-byte checksummed packet:

```
[0xAA] [sensorType:1] [state:1] [seq:1] [checksum:1]
  |         |             |        |         |
 start   0=PIR          0/1     rolling   XOR of sensorType,
 byte    1=Door                 counter   state, seq
         2=Glassbreak
```

The ESP32 recomputes the XOR checksum over `sensorType, state, seq` and discards any packet that doesn't match, then discards any packet not starting with `0xAA`.

**WiFi (ESP32 → MQTT):** on a valid alert, the ESP32 publishes a JSON payload to topic `security/alerts` on a configured broker:

```json
{"sensor":"PIR","state":1,"seq":42}
```

Credentials are never hardcoded — see the placeholder block at the top of `src/esp32-relay/esp32_relay.ino`:

```cpp
// ---- credentials placeholder — replace before use, never commit real secrets ----
const char* WIFI_SSID = "YOUR_WIFI_SSID";
const char* WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";
const char* MQTT_BROKER = "YOUR_MQTT_BROKER_IP_OR_HOST";
const int   MQTT_PORT = 1883;
const char* MQTT_TOPIC = "security/alerts";
// ---------------------------------------------------------------------------------
```

## Setup & Deployment

1. Wire the Mega's 3 sensors, local buzzer, and `Serial2` link to the ESP32 as above.
2. Install `PubSubClient` (by Nick O'Leary) on the ESP32 side only (see `libraries.txt`); the Mega side needs no external libraries.
3. Edit `src/esp32-relay/esp32_relay.ino`'s credentials placeholder block with your actual WiFi SSID/password and MQTT broker address — do not commit real credentials to source control.
4. Flash `src/esp32-relay/esp32_relay.ino` to the ESP32 DevKit first (select the correct ESP32 board profile in the Arduino IDE/PlatformIO).
5. Flash `src/arduino-sensors/security_fusion.ino` to the Mega.
6. Power both devices. Confirm the ESP32's Serial Monitor shows a successful WiFi + MQTT broker connection.
7. Trigger the PIR, open the door reed switch, or tap the vibration sensor, and confirm: (a) the Mega's local buzzer sounds immediately, and (b) the ESP32 prints a received/validated packet and publishes to the configured MQTT topic — subscribe to `security/alerts` with any MQTT client (e.g. `mosquitto_sub`) to confirm end-to-end delivery.

## Known Limitations & Path to Production

- The UART link between Mega and ESP32 has no retry/ack — a dropped packet (e.g. a momentary disconnect) is simply lost; production would add the same ACK/retry pattern used elsewhere in this tier's multi-board projects.
- MQTT publish uses no TLS and (by default) no broker authentication — production should use MQTT over TLS with broker credentials, and `PubSubClient`'s `setServer`/`connect` calls extended accordingly.
- No local persistence — if the ESP32 loses WiFi, in-flight alerts are dropped rather than queued for later delivery.

## Extension Ideas

- Add store-and-forward buffering on the ESP32 (e.g. to its flash/SPIFFS) so alerts survive a temporary WiFi outage.
- Add MQTT-over-TLS with broker username/password authentication.
- Add a webhook fallback path (HTTPS POST) alongside MQTT for services that don't run their own broker.
