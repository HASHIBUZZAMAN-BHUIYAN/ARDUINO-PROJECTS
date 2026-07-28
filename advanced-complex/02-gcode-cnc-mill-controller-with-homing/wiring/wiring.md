# Wiring Notes — G-Code CNC Mill Controller

```
Mega              A4988 (x3, one per axis)          NEMA17 (x3)
+---------+      +----------------+                +----------+
| D22/24/26|-----| STEP           |----coils to---> | motor    |
| D23/25/27|-----| DIR            |                 +----------+
|      5V |------| VDD            |
|     GND |------| GND (logic)    |
+---------+      | VMOT <- 12V PSU|
                  | GND  <- PSU GND|
                  +----------------+

Mega                Limit switches (x3, NO to GND, INPUT_PULLUP)
+---------+        +------------------------------+
|      D2 |--------| X limit (normally open)      |
|      D3 |--------| Y limit                      |
|     D18 |--------| Z limit                      |
+---------+        +------------------------------+

Mega                Spindle relay          SD module (hw SPI)
+---------+        +-------------+        +--------------+
|     D28 |--------| IN          |    53--| CS            |
|      5V |--------| VCC         |    51--| MOSI          |
|     GND |--------| GND         |    50--| MISO          |
+---------+        +-------------+    52--| SCK           |
                                          +--------------+
```

- Stepper driver logic (VDD/GND) and motor power (VMOT/GND) are separate rails — never power VMOT from the Mega's 5V pin.
- Limit switches are wired normally-open to GND with `INPUT_PULLUP` enabled in software, so an unplugged/broken switch reads as "not triggered" (safe default) rather than falsely triggering.
- Set each A4988's current-limit trimpot before first use — an unset pot can overheat and destroy both the driver and the motor within seconds.
