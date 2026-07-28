# Architecture — Multi-Sensor Weather Buoy Datalogger

## System Diagram

```
 +-----------+  +-----------+  +-----------+  +-----------+  +-----------+  +-----------+
 |Anemometer |  |Wind vane  |  |Rain gauge |  |BME280      |  |UV sensor  |  |Ultrasonic |
 |(pulses,INT)|  |(analog)   |  |(pulses,INT)|  |(I2C)       |  |(analog)   |  |(level)    |
 +-----+-----+  +-----+-----+  +-----+-----+  +-----+-----+  +-----+-----+  +-----+-----+
       |              |              |              |              |              |
       +------+-------+------+-------+------+-------+------+-------+------+-------+
                     v
           +-----------------------+
           |  Uno Q fusion + rollup |
           |  - 5-min snapshot       |
           |  - daily min/max/avg    |
           +-----------+-------------+
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

1. **Count** — anemometer and rain-gauge reed switches trigger interrupt handlers that increment pulse counters continuously.
2. **Sample** — every 5 minutes, `buildSnapshot()` converts pulse counts (since last snapshot) into wind speed and rainfall-since-last-snapshot, reads the wind vane, BME280, UV sensor, and ultrasonic water level.
3. **Roll up** — each snapshot's values update running daily min/max/sum/count accumulators.
4. **Persist** — the snapshot is appended to `WEATHER.CSV`; at local midnight (detected via RTC), a summary row (day's min/max/avg per field) is appended and accumulators reset.
5. **Serve** — the W5500 HTTP server answers `/api/data` from the latest snapshot and `/api/history` by re-reading the SD log's tail.
6. **Display** — the dashboard polls both endpoints and renders current conditions plus a sparkline of recent history.

## Component Roles

- **Anemometer / rain gauge** — interrupt-driven pulse sources, the two sensors that specifically require careful ISR-based counting rather than simple polling.
- **Wind vane / BME280 / UV / ultrasonic** — polled analog/I2C sensors read once per snapshot.
- **Uno Q** — owns fusion, rollup accumulation, SD persistence, and the HTTP server.
- **RTC** — authoritative clock for both snapshot timestamps and the daily-rollup boundary.
- **SD** — durable historical record independent of network availability.
- **W5500** — LAN-facing live + recent-history view.
