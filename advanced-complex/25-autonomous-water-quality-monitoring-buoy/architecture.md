# Architecture — Autonomous Water Quality Monitoring Buoy

## System Diagram

```
 +-----+ +-----------+ +-----------+ +-----------+ +-----------+ +-----------+
 | pH  | | Turbidity  | | Dissolved  | | DS18B20    | | HC-SR04    | | INA219     |
 |     | |            | | Oxygen      | | (water temp)| | (level)     | | (power)     |
 +--+--+ +-----+-----+ +-----+-----+ +-----+-----+ +-----+-----+ +-----+-----+
    |          |             |             |             |             |
    +----+-----+------+------+------+------+------+------+------+------+
              v
    +-----------------------+
    |  Uno Q anomaly engine   |
    |  - threshold + rate-of-  |
    |    change checks          |
    |  - rolling hourly stats    |
    +------------+---------------+
                 v
      +--------------------+       +------------------+
      | Aerator + alarm       |     | SD (RTC ts,        |
      | relays (closed loop)   |     | hourly rollups)      |
      +--------------------+       +----------+----------+
                                                v
                                    +------------------------+
                                    | W5500 token-auth REST     |
                                    +-----------+---------------+
                                                v
                                    +------------------------+
                                    | dashboard/*.html          |
                                    +------------------------+
```

## Data Flow

1. **Sense** — every cycle, read pH, turbidity, DO, water temperature, water level, and the INA219 power monitor.
2. **Analyze** — the anomaly engine checks each value against static thresholds and, for DO specifically, its rate of change over recent samples (a fast drop is the actionable "bloom/fish-kill risk" signal, not just an absolute low reading).
3. **Act (closed loop)** — a confirmed anomaly engages the aerator relay and shore alarm; both disengage once conditions recover past a hysteresis band.
4. **Roll up** — readings accumulate into rolling hourly min/max/avg accumulators, flushed to SD as a summary row every hour.
5. **Authenticate + serve** — the token-gated HTTP server answers `/api/state` and `/api/history`.
6. **Display** — the dashboard renders live readings, power status, and historical trend charts.
7. **Recover** — a software watchdog resets the board if the main loop hangs, keeping the buoy self-healing during unattended deployment.

## Component Roles

- **pH / turbidity / DO / temp / level sensors** — the water-quality sensing fusion layer (5 distinct signals).
- **INA219** — power-autonomy monitoring, relevant because this buoy is meant to run unattended on solar/battery.
- **Anomaly engine** — the closed-loop decision layer, the platform's automation trait.
- **SD + RTC** — historical rollups for trend analysis.
- **W5500 + token auth** — remote visibility and the security-layer trait, assuming shore connectivity.
- **Watchdog** — the reliability trait essential for a device nobody is physically checking on daily.
