# Architecture — Smart Agriculture Command Platform

## System Diagram

```
 8x Soil probes --> CD74HC4067 mux --> A0     BME280 / BH1750 (I2C climate/light)
                          |                          |
                          +-----------+--------------+
                                      v
                        +--------------------------+
                        |  Mega control core          |
                        |  - irrigation closed loop     |
                        |  - climate PID                 |
                        |  - RTC schedule                  |
                        |  - watchdog (auto-recover)         |
                        +------------+---------------------+
                                     v
                +--------------------+--------------------+
                v                                          v
      +--------------------+                    +--------------------+
      | Valve + climate       |                    | SD "database"       |
      | relays (actuation)     |                    | (structured CSV)     |
      +--------------------+                    +----------+----------+
                                                            v
                                                 +------------------------+
                                                 | W5500 token-auth REST   |
                                                 +-----------+-------------+
                                                             v
                                                 +------------------------+
                                                 | dashboard/*.html (token) |
                                                 +------------------------+
```

## Data Flow

1. **Sense** — every control tick, read all 8 multiplexed soil zones and the climate/light sensors.
2. **Control** — independent irrigation closed-loop logic (per zone, moisture-threshold + timeout) and climate PID (temperature/humidity) run each tick; the RTC gates the grow-light schedule.
3. **Persist** — every irrigation event and periodic climate snapshot is appended as a structured CSV row to SD, forming the platform's "database."
4. **Query (local)** — a serial `QUERY` command scans the CSV for matching rows, printed to the console — a lightweight, no-network way to inspect history.
5. **Authenticate + serve (remote)** — the W5500 HTTP server checks `X-Auth-Token` on every request before answering `/api/state`, `/api/history`, or accepting a zone/setpoint change.
6. **Display** — the dashboard prompts for the token once, then polls the authenticated endpoints and renders live state plus historical charts.
7. **Recover** — the watchdog timer resets the board automatically if the main loop ever stops feeding it, restoring the platform to a known-good state without manual intervention.

## Component Roles

- **Multiplexer + soil probes** — the platform's widest sensing fan-out, the "custom-PCB-worthy wiring" trait.
- **Climate sensors + PID** — the second independent control domain running alongside irrigation.
- **SD** — durable structured storage, queryable both locally (serial) and remotely (REST history).
- **W5500 + token auth** — the platform's security-layer trait, gating all remote read/write access.
- **Watchdog** — the platform's reliability/production-hardening trait.
