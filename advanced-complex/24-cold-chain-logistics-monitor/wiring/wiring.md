# Wiring Notes — Cold Chain Logistics Monitor

```
Mega                 DS18B20 x3 (OneWire bus)      DHT22           ADXL345 (I2C)
+--------+          +------------------------+    +--------+     +-----------+
|     D2 |----------| DATA (shared, unique     |    |        |     |           |
|        |          | ROM addr per probe)       |    |        |     |           |
|     D3 |------------------------------------- --| DATA   |     |           |
| 20/21  |------------------------------------------------------ --| SDA/SCL   |
+--------+          +------------------------+    +--------+     +-----------+

Mega                 Door reed         Tamper switch     Cooling relay    Buzzer
+--------+          +------------+     +--------------+ +--------------+ +--------+
|     D4 |----------| (INPUT_PULLUP)   |
|     D5 |------------------------------ -| (INPUT_PULLUP)|
|     D6 |------------------------------------------------- -| IN            |
|     D7 |------------------------------------------------------------------ -| +      |
+--------+          +------------+     +--------------+ +--------------+ +--------+

Mega                 SIM800L                SD (hw SPI)
+--------+          +----------------+     +------------+
| D16(TX2)|----------| RX             |     |            |
| D17(RX2)|----------| TX             |     |            |
|         |          | VCC (separate 4V |     |            |
|         |          | regulated supply, |     |            |
|         |          | NOT the Mega's 5V)|     |            |
|     53  |------------------------------ --| CS         |
| 50/51/52|------------------------------ --| MISO/MOSI/SCK|
+--------+          +----------------+     +------------+
```

- The SIM800L draws current spikes up to ~2A during transmission bursts — power it from a dedicated regulated 3.7-4.2V supply capable of that peak, never directly from the Mega's 5V rail.
- Each DS18B20 shares one OneWire data line (D2) with a single 4.7kΩ pull-up; each probe has a unique factory-set 64-bit ROM address used in software to distinguish readings (run a one-time address-scan sketch to discover them).
- Mount the ADXL345 rigidly to the container/vehicle structure, not to a cushioned surface, for meaningful shock-event detection.
