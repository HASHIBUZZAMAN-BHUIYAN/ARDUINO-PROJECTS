# Wiring Notes — Security Fusion with ESP32 WiFi Relay

```
Mega (Sensor Fusion)        PIR (HC-SR501)    Door reed switch    Piezo vibration sensor
+--------+                 +------------+    +------------------+ +----------------------+
|     D2 |-----------------| OUT        |    |                  | |                      |
|     D3 |--------------------------------- --| (INPUT_PULLUP)   | |                      |
|     A0 |------------------------------------------------------------ -| AOUT                |
|     D4 |-----------------------------------------------------------------------------------| (buzzer, separately) |
+--------+                 +------------+    +------------------+ +----------------------+

Mega                          ESP32 DevKit
+--------+                   +--------+
| D16(TX2)|------------------>| GPIO16 (RX2) |
| D17(RX2)|<------------------| GPIO17 (TX2) |
|   GND   |------------------>|    GND       |
+--------+                   +--------+
```

- The Mega's `Serial2` (D16/D17) and the ESP32's second hardware UART (GPIO16/17) are crossed: Mega TX2 to ESP32 RX2, and Mega RX2 to ESP32 TX2, with a shared ground.
- The ESP32 runs on 3.3V logic; the Mega's UART pins are 5V-tolerant-adjacent but best practice is a simple voltage divider or logic-level shifter on the Mega-TX2 -> ESP32-RX2 line if you want to be fully within the ESP32's 3.3V input spec.
- Keep the piezo vibration sensor's comparator threshold (usually a small onboard potentiometer on cheap breakout boards) tuned so normal ambient vibration doesn't false-trigger.
