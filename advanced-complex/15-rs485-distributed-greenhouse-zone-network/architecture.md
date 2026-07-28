# Architecture — RS-485 Distributed Greenhouse Zone Network

## System Diagram

```
 +----------------+  +----------------+  +----------------+
 | Zone 1 (Uno)    |  | Zone 2 (Uno)    |  | Zone 3 (Uno)    |
 | local PID loop  |  | local PID loop  |  | local PID loop  |
 | (independent)   |  | (independent)   |  | (independent)   |
 +-------+--------+  +-------+--------+  +-------+--------+
         |                    |                    |
         +---------+----------+----------+----------+
                   |   RS-485 bus (supervisory only)  |
                   v
         +--------------------------+
         |  Mega Hub (RS-485 master) |
         |  - poll + log             |
         |  - setpoint push           |
         +------------+---------------+
                      v
              +------------------+
              | SD log (RTC ts)   |
              +--------+---------+
                        v
              +------------------+
              | W5500 REST API    |
              +--------+----------+
                        v
              +------------------+
              | dashboard/*.html  |
              +------------------+
```

## Data Flow

1. **Local control (each zone, always running)** — regardless of the bus, each zone reads its own DHT22 + soil probe every control tick and runs its own PID loop against its currently-held setpoint, driving its own heater/fan/mister relays.
2. **Supervisory poll (hub)** — every 10 seconds, the hub polls each zone in turn for its current readings and actuator state.
3. **Persist** — the hub logs each successful poll to SD with an RTC timestamp.
4. **Setpoint push (operator-driven)** — when an operator changes a setpoint via the dashboard, the hub sends a `SET` command to the relevant zone over RS-485; the zone immediately adopts it for its local loop.
5. **Serve** — the hub's HTTP server answers `/api/zones` from its latest polled snapshot and accepts setpoint-push requests.
6. **Display** — the dashboard renders all 3 zones and lets an operator adjust setpoints.

## Component Roles

- **Zone controllers** — the actual control authority for their own zone; designed to keep working correctly with zero bus traffic.
- **Hub** — purely supervisory: visibility (logging, dashboard) and setpoint adjustment, never in the zones' real-time control path.
- **RS-485 bus** — the low-rate, non-critical link between supervisory and local control layers, matching how real industrial distributed control systems (DCS) separate fast local loops from slower supervisory networks.
