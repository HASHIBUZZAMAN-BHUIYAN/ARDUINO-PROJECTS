# Architecture — RS-485 Energy Monitoring Mesh

## System Diagram

```
 +---------------+  +---------------+  +---------------+
 | Meter Node 1  |  | Meter Node 2  |  | Meter Node 3  |
 | CT + voltage   |  | CT + voltage   |  | CT + voltage   |
 +-------+-------+  +-------+-------+  +-------+-------+
         |                   |                   |
         +---------+---------+---------+---------+
                   |    RS-485 bus (poll cycle)     |
                   v
         +--------------------------+
         |  Mega Hub (RS-485 master) |
         |  - poll + accumulate Wh    |
         |  - SD log (RTC ts)          |
         +------------+---------------+
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

1. **Sample (each node)** — synchronized ADC reads of the CT clamp and voltage sensor across a mains cycle let `computeRealPower()` multiply instantaneous voltage x current samples and average, correctly handling any phase offset between them (unlike naively multiplying RMS voltage by RMS current, which overstates power for non-unity power-factor loads).
2. **Accumulate (each node)** — instantaneous watts are integrated over time into a running watt-hour total since node boot.
3. **Poll (hub)** — the hub requests each node's instantaneous watts and cumulative Wh every 5 seconds over RS-485.
4. **Persist** — each successful poll is appended to SD with an RTC timestamp.
5. **Serve** — the W5500 HTTP server answers `/api/circuits` with the latest per-circuit snapshot.
6. **Display** — the dashboard renders live per-circuit wattage and a simple daily-total view.

## Component Roles

- **CT clamp + voltage sensor** — the sensing pair each node needs to compute true real power rather than just current magnitude.
- **Meter nodes** — independent per-circuit measurement and accumulation.
- **Mega hub** — bus master, historical logger, and dashboard server; not involved in any node's own measurement accuracy.
- **RTC + SD** — durable historical record for later cost/usage analysis.
