# Wiring Notes — Redundant Dual-MCU Failover Monitoring System

```
                Shared sensor wiring (identical on both boards)
DS18B20 --+-------------------+
          |                   |
        Primary D2          Standby D2   (shared 4.7k pull-up on the OneWire line)

MQ-2 AOUT --+-------------------+
            |                   |
          Primary A0          Standby A0

Water leak probe --+-------------------+
                    |                   |
                  Primary D3          Standby D3

                Shared actuator wiring (identical on both boards)
Siren relay IN --+-------------------+
                  |                   |
                Primary D4          Standby D4   (only the ACTIVE board's D4 is OUTPUT)

Valve relay IN --+-------------------+
                  |                   |
                Primary D5          Standby D5   (only the ACTIVE board's D5 is OUTPUT)

Primary (Uno)                Standby (Uno)
+--------+                  +--------+
|  D6(TX)|----------------->| D7(RX) |
|  D7(RX)|<-----------------| D6(TX) |
|   GND  |----------------->|  GND   |
+--------+                  +--------+
```

- The critical safety wiring rule: at any moment, exactly one board's D4/D5 pins must be `OUTPUT` and the other's must be `INPUT` — never both `OUTPUT` at once, which would let two boards drive the same relay coil in conflicting directions.
- A shared 4.7kΩ pull-up resistor on the OneWire (DS18B20) line works for both boards reading it in parallel.
- Test the failover behavior by physically unplugging Primary's power, not just resetting it, to properly exercise the "heartbeat silence" detection path.
