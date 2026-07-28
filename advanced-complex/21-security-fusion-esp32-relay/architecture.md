# Architecture — Security Fusion with ESP32 WiFi Relay

## System Diagram

```
 +-----------+  +-----------+  +------------------+
 | PIR       |  | Door reed |  | Piezo vibration    |
 | (motion)  |  | switch    |  | (glass-break)       |
 +-----+-----+  +-----+-----+  +---------+-----------+
       |              |                   |
       +------+-------+-------------------+
              v
     +--------------------------+
     |  Mega "Sensor Fusion"      |
     |  - trigger detection        |
     |  - local buzzer              |
     |  - checksummed UART packet    |
     +-------------+----------------+
                   | UART (9600 baud, [0xAA][type][state][seq][chk])
                   v
     +--------------------------------+
     |  ESP32 "WiFi Relay"               |
     |  - packet validation                |
     |  - WiFi connect                      |
     |  - MQTT publish (PubSubClient)        |
     +-------------------+--------------------+
                         v
               +----------------------+
               | MQTT broker            |
               | topic: security/alerts |
               +----------------------+
```

## Data Flow

1. **Sense** — the Mega polls PIR (digital), door reed switch (digital), and the piezo vibration sensor (analog, compared against a threshold) every loop iteration.
2. **Detect trigger** — any sensor crossing its trigger condition (PIR high, reed switch open, vibration above threshold) immediately sounds the local buzzer and queues an alert.
3. **Package** — `sendAlert()` builds the 5-byte packet (start byte, sensor type, state, sequence, XOR checksum) and transmits it once over `Serial2`.
4. **Relay** — the ESP32's `readPacket()` validates the start byte and checksum, discarding anything malformed.
5. **Publish** — on a valid packet, `connectIfNeeded()` ensures WiFi + MQTT connectivity, then `publishAlert()` sends a JSON payload to the configured MQTT topic.
6. **Consume (external)** — any MQTT subscriber (a dashboard, a phone-notification bridge, etc., out of scope for this repo) can act on the published alert.

## Component Roles

- **Mega** — the sole sensing and local-response authority (buzzer); has no network stack of its own by design, keeping the AVR side simple and offline-capable even without WiFi.
- **ESP32** — the sole network-facing component; has no direct sensor wiring, receiving everything it knows about the physical world through the UART link.
- **UART link** — the fixed protocol boundary between the two devices, deliberately simple (a single small packet type) since it only needs to carry discrete alert events, not continuous telemetry.
- **MQTT broker** — external to this repo; any standard broker (Mosquitto, a cloud MQTT service, etc.) can receive the published alerts.
