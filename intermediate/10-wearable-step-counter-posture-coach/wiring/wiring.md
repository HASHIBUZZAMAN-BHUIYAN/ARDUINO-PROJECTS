# Wiring Notes — Wearable Step Counter & Posture Coach

```
Arduino Nano              MPU6050
+-----------+             +----------+
|        5V |-------------| VCC      |
|       GND |----+--------| GND      |
|     A4/SDA|-------------| SDA      |
|     A5/SCL|-------------| SCL      |
+-----------+             +----------+
                  |
Arduino Nano      |         Vibration motor driver
+-----------+     |        +--------------------------+
|         D9|--[220ohm]----| Transistor/MOSFET base    |
+-----------+              | Collector/Drain -> motor - |
                            | Emitter/Source -> GND      |
                            +--------------------------+
   Motor + -> 5V (or dedicated battery +)
   1N4001 diode across the motor terminals, banded end toward +,
   to absorb the motor's back-EMF and protect the transistor.
```

- Mount the MPU6050 as flat and centered on the wrist/belt as practical — its orientation relative to your body directly affects which axes correspond to "forward lean," so the posture-angle math may need its axis choice adjusted for your specific mounting.
- For an actual wearable build, replace the breadboard with a small perfboard or protoboard and power from a 3.7V LiPo through a boost/charge module rated for the Nano's input.
