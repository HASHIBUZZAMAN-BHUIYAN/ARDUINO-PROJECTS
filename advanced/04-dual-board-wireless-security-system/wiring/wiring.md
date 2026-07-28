# Wiring Notes — Dual-Board Wireless Security System

```
REMOTE SENSOR NODE (Arduino Nano)

Nano                      nRF24L01+
+-----------+            +----------+
|       3V3 |------------| VCC      |  (3.3V only, never 5V)
|       GND |----+-------| GND      |
|        D9 |------------| CE       |
|       D10 |------------| CSN      |
|       D13 |------------| SCK      |
|       D11 |------------| MOSI     |
|       D12 |------------| MISO     |
+-----------+            +----------+
                 |
Nano             |         PIR (HC-SR501)         Reed switch
+-----------+    |        +----------+            +------------------+
|        5V |-------------| VCC      |   5V -------| one leg          |
|         D2|------------| OUT      |   A0 --------| other leg        |
+-----------+                                       A0 --10k-- GND (pull-down)
                                                    +------------------+


BASE STATION (Arduino Uno)

Uno                        nRF24L01+
+-----------+            +----------+
|       3V3 |------------| VCC      |
|       GND |----+-------| GND      |
|        D9 |------------| CE       |
|       D10 |------------| CSN      |
|       D13 |------------| SCK      |
|       D11 |------------| MOSI     |
|       D12 |------------| MISO     |
+-----------+            +----------+

Uno                        16x2 LCD
+-----------+            +----------+
|        D2 |------------| RS       |
|        D3 |------------| EN       |
|        D4 |------------| D4       |
|        D5 |------------| D5       |
|        D6 |------------| D6       |
|        D7 |------------| D7       |
+-----------+            +----------+
   Contrast pot wiper -> LCD V0; pot outer legs -> 5V/GND

     D8 ----------------- Buzzer + (- to GND)
```

- nRF24L01 modules are strictly 3.3V on VCC, even though their signal pins tolerate the Uno/Nano's 5V logic levels on most breakout revisions — always power VCC from 3V3.
- If a radio never seems to receive anything, add a 10-100uF capacitor across the nRF24L01's 3V3/GND pins close to the module — many cheap boards are sensitive to power noise, and this is the most common fix for unreliable links.
- Both boards must use the exact same 5-byte pipe address string in their sketches (already matched in the provided `remote_node.ino` / `base_station.ino`) to talk to each other.
