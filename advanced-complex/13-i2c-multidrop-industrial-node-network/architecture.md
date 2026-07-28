# Architecture — I2C Multi-Drop Industrial Node Network

## System Diagram

```
                 Shared I2C bus (SDA/SCL + GND, one pull-up pair)
   +----------------+----------------+----------------+
   |                |                |                |
+--+-------+   +----+-----+    +-----+------+    +----+-----+
| Relay    |   | Load Cell |    | Thermo-    |    | Uno Q    |
| Bank     |   | (HX711)   |    | couple     |    | Hub      |
| (0x08)   |   | (0x09)    |    | (MAX6675)  |    | (master) |
+----------+   +-----------+    | (0x0A)     |    +----+-----+
                                  +-----------+          |
                                                          v
                                                +------------------+
                                                | SD log (RTC ts)   |
                                                +--------+---------+
                                                          v
                                                +------------------+
                                                | W5500 dashboard    |
                                                +------------------+
```

## Data Flow

1. **Poll** — every 2 seconds, the hub issues `Wire.requestFrom()` to each node address in turn.
2. **Node reply** — each Nano's `receiveEvent`/`requestEvent` I2C-slave callback returns its latest cached reading (relay states, weight, or temperature) as a compact struct.
3. **Actuate** — a relay change is a separate `Wire.write()` transmission the hub sends to `0x08` outside the normal poll cycle, immediately in response to a dashboard button press.
4. **Persist** — the hub appends a timestamped row per successful poll to SD.
5. **Serve** — the W5500 HTTP server answers `/api/nodes` with the latest in-RAM snapshot of all 3 nodes.
6. **Display** — the dashboard renders live values and relay toggle controls.

## Component Roles

- **Relay bank node** — the only actuator on the bus; exposes a settable bitmask.
- **Load cell / thermocouple nodes** — pure sensor nodes, each wrapping a different local sensor interface (HX711 bit-bang, MAX6675 software SPI) behind the same I2C read interface.
- **Uno Q hub** — sole bus master, owns polling cadence, persistence, and the dashboard.
- **RTC + SD** — durable historical record.
