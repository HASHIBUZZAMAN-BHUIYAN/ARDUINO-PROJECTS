# Architecture — Home Energy Resilience Management System

## System Diagram

```
 +-----------+  +-----------+  +-----------+
 | Grid leg  |  | Solar leg |  | Battery leg|
 | INA219    |  | INA219    |  | INA219     |
 +-----+-----+  +-----+-----+  +-----+-----+
       |              |              |
       +------+-------+------+-------+
                     v
           +-----------------------+
           |  Mega decision engine   |
           |  - source selection      |
           |  - load-shedding ladder   |
           +-----------+---------------+
                       v
         +--------------------------+
         | 4-ch relay (transfer +    |
         | load shedding)             |
         +--------------------------+
                       |
                       v
           +-----------------------+
           |  EEPROM (state persist)|
           +-----------------------+
                       v
           +-----------------------+
           |  SD (daily/weekly       |
           |  rollups, RTC ts)         |
           +-----------+-------------+
                       v
             +------------------+
             | W5500 token-auth   |
             | REST API             |
             +---------+-----------+
                       v
             +------------------+
             | dashboard/*.html   |
             +------------------+
```

## Data Flow

1. **Sense** — read all 3 INA219 legs every control tick.
2. **Decide** — `selectSource()` prefers solar, falls back to battery, then grid, engaging the transfer relay accordingly; `updateLoadShedding()` separately compares battery SoC against a priority ladder of thresholds to shed or restore non-critical loads.
3. **Persist (immediate)** — every relay-state or meaningful Wh-counter change is written to EEPROM right away, not just periodically, so a power loss mid-operation loses no state.
4. **Persist (historical)** — a rollup routine accumulates daily min/max/avg per leg and writes a summary row to SD at local midnight.
5. **Authenticate + serve** — the token-gated HTTP server answers `/api/state` and `/api/history` for the dashboard.
6. **Display** — the dashboard renders live power flow and historical rollup charts.

## Component Roles

- **3x INA219** — the platform's entire sensing layer; simple but sufficient for source-selection and SoC-based decisions.
- **Decision engine** — the closed-loop automation core; a real, non-trivial priority-ordered load-shedding ladder.
- **EEPROM** — the production-hardening layer: state survives power loss, which is the whole point of a resilience system.
- **SD + RTC** — historical record for post-hoc analysis of source usage and outage patterns.
- **W5500 + token auth** — remote visibility and the security-layer trait.
