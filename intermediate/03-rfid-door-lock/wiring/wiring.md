# Wiring Notes — RFID Door Lock

```
Arduino Nano              MFRC522
+-----------+             +----------+
|       3V3 |-------------| VCC      |  (3.3V ONLY, never 5V)
|       GND |----+--------| GND      |
|       D10 |-------------| SDA(SS)  |
|       D13 |-------------| SCK      |
|       D11 |-------------| MOSI     |
|       D12 |-------------| MISO     |
|        D9 |-------------| RST      |
+-----------+             +----------+

Arduino Nano              Lock servo
+-----------+             +----------+
|        5V |-------------| VCC      |
|       GND |-------------| GND      |
|        D5 |-------------| signal   |
+-----------+             +----------+

     D6-[220ohm]--|>|-- Green LED --GND (access granted)
     D7-[220ohm]--|>|-- Red LED   --GND (access denied)
     D8 ----------------- Buzzer + (- to GND) (denied beep)
```

- MFRC522 is strictly a 3.3V part; connecting VCC to 5V can damage it.
- SPI pins (SCK/MOSI/MISO) are fixed by the Nano's hardware SPI bus — only SDA(SS) and RST are freely reassignable in software.
- Mount the servo so its horn rotates a physical bolt/latch between "locked" and "unlocked" positions; calibrate the two angles in the sketch to match your hardware.
