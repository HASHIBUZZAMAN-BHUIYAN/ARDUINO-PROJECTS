# Architecture — Multiplexed 8-Zone Soil Irrigation Scheduler

## System Diagram

```
 8x Soil Probes --> +--------------------+
                     | CD74HC4067 mux     |--(SIG)--> Mega A0
                     | (S0-S3 select)     |
                     +---------+----------+
                               ^ (select lines D22-D25)
                               |
 Rain sensor -----------------+
                               |
                     +--------------------+
                     |  Mega scheduler     |
                     |  - RTC schedule      |
                     |  - moisture close-   |
                     |    loop shutoff      |
                     +----+-----------+-----+
                          v           v
                +------------------+ +------------------+
                | 8-ch relay ->     | | SD log (RTC ts)   |
                | solenoid valves   | +--------+---------+
                +------------------+          v
                                     +------------------+
                                     | W5500 REST API    |
                                     +--------+----------+
                                              v
                                     +------------------+
                                     | dashboard/*.html  |
                                     +------------------+
```

## Data Flow

1. **Select** — `readZone(n)` sets the 4 select lines to route zone `n`'s probe onto A0, then reads it.
2. **Schedule check** — every minute, `checkSchedules()` compares the RTC's current time against each zone's configured window and the rain sensor's state.
3. **Open** — if a window is due and no recent rain, the zone's valve relay energizes and a per-zone `runningSince` timestamp is recorded.
4. **Close (closed loop)** — while a valve is open, its zone is re-read each cycle; the valve de-energizes as soon as the moisture reading crosses the target threshold, or when `maxRuntimeMs` elapses, whichever comes first.
5. **Persist** — every watering start/stop and periodic soil snapshot is appended to `IRRIGLOG.CSV` on SD with an RTC timestamp.
6. **Serve** — the W5500 HTTP server answers `/api/zones` from the in-RAM zone state array and accepts `/api/zones/<n>/on|off` overrides.

## Component Roles

- **CD74HC4067** — turns 8 analog sensors into 1 analog pin + 4 digital selects, the project's core wiring-complexity trait.
- **Rain sensor** — a weather-adaptive gate on the schedule, preventing wasted watering after rain.
- **DS3231 RTC** — authoritative source for both scheduling windows and log timestamps.
- **Mega** — owns the scheduling decision, the closed-loop shutoff logic, and the HTTP server.
- **Relay + solenoids** — the actuator layer the closed loop drives.
- **SD + W5500** — durable history and live/remote visibility, respectively.
