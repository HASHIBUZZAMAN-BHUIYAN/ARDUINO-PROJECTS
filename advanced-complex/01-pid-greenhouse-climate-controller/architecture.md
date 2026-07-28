# Architecture — PID Greenhouse Climate Controller

## System Diagram

```
 +--------------+     +---------------+     +----------------+
 |  DHT22       |     |  BH1750       |     |  Soil probe    |
 |  temp/hum    |     |  light (I2C)  |     |  (analog)      |
 +------+-------+     +-------+-------+     +--------+-------+
        |                     |                       |
        v                     v                       v
      +---------------------------------------------------+
      |            Arduino Mega 2560 (control core)        |
      |  - Temp PID  -> heater/fan relays                   |
      |  - Humidity PID -> mister relay                     |
      |  - RTC schedule -> grow-light relay                 |
      +----------------------+------------------------------+
                              |
              +---------------+----------------+
              v                                 v
      +---------------+                 +----------------+
      |  microSD card |                 |  W5500 Ethernet |
      |  CSV log      |                 |  HTTP server    |
      +---------------+                 +--------+--------+
                                                  |
                                                  v
                                        +--------------------+
                                        |  Browser dashboard |
                                        |  (dashboard/*.html)|
                                        +--------------------+
```

## Data Flow

1. **Sample** — every control tick (2s), read DHT22, BH1750, and soil probe.
2. **Control** — `updatePID()` computes heater/fan/mister outputs from the temperature and humidity PID loops; `updateSchedule()` checks the RTC against the configured grow-light window.
3. **Actuate** — relay pins are set from PID output signs/thresholds (bang-bang around the PID error band, since these are on/off relays rather than PWM-driven elements).
4. **Persist** — once a minute, `logRow()` appends a timestamped CSV row (readings + PID outputs + relay states) to `GREENHOUSE.CSV` on the SD card.
5. **Serve** — `handleClient()` answers `GET /api/data` with the latest in-RAM snapshot as JSON, and `GET /setpoint` mutates the PID setpoints in RAM.
6. **Display** — `dashboard/index.html`, opened in any browser on the LAN, polls `/api/data` every 3s and renders live values plus a small setpoint form.

## Component Roles

- **DHT22 / BH1750 / soil probe** — process variables for the two PID loops and the scheduling check.
- **Mega 2560** — runs both PID loops, the light schedule, SD logging, and the HTTP server; the single point of control.
- **4-channel relay module** — actuates heater, fan, mister, and grow-light from the Mega's digital outputs.
- **DS3231 RTC** — authoritative time source for both CSV timestamps and the grow-light schedule.
- **microSD card** — durable historical record independent of network availability.
- **W5500 shield** — exposes live state and setpoint control to the LAN.
