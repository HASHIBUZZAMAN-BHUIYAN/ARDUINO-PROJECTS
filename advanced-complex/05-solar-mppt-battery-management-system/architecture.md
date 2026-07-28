# Architecture — Solar MPPT Battery Management System

## System Diagram

```
 +----------------+       +------------------+
 |  Solar panel   |       |  Battery bank     |
 +-------+--------+       +---------+---------+
         |                          |
         v                          v
 +----------------+       +------------------+
 |  INA219 (panel)|       |  INA219 (battery)|
 +-------+--------+       +---------+---------+
         |                          |
         +-------------+------------+
                       v
             +--------------------+
             |  Uno Q control core |
             |  - P&O MPPT loop     |
             |  - coulomb counting  |
             +----+-----------+-----+
                  v           v
         +----------------+ +----------------+
         |  PWM -> buck    | |  SD log + RTC  |
         |  converter gate | |                |
         +----------------+ +--------+--------+
                                      v
                            +------------------+
                            |  W5500 HTTP API   |
                            +--------+----------+
                                     v
                            +------------------+
                            | dashboard/*.html  |
                            +------------------+
```

## Data Flow

1. **Sense** — read panel and battery voltage/current from the two INA219s every 500ms control tick.
2. **Track** — `perturbAndObserve()` compares this tick's panel power to last tick's; if increasing duty cycle increased power last time, keep increasing it, otherwise reverse direction — climbing toward the maximum power point.
3. **Actuate** — the new duty cycle is written to the PWM pin driving the buck converter's MOSFET gate.
4. **Integrate** — battery current is multiplied by elapsed time and accumulated into a running charge total, converted to a state-of-charge percentage against the configured battery capacity.
5. **Persist** — once a minute, `logRow()` appends a timestamped CSV row of all four readings, duty cycle, and SoC to SD.
6. **Serve** — the W5500 HTTP server answers `/api/data` from the latest in-RAM snapshot and `/api/history` by re-reading the tail of the SD log on demand.

## Component Roles

- **INA219 (panel)** — process variable for the MPPT loop.
- **INA219 (battery)** — process variable for coulomb counting and the dashboard's battery view.
- **Uno Q** — runs both the fast control loop (MPPT) and the slower logging/serving loop.
- **Buck converter stage** — the actuator the MPPT loop closes its loop around.
- **SD + RTC** — historical record independent of network availability.
- **W5500 shield** — the LAN-facing view into current and historical state.
