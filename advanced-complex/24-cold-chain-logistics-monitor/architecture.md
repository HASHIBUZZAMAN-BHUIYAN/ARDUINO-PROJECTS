# Architecture — Cold Chain Logistics Monitor

## System Diagram

```
 +-----------+ +-----------+ +-----------+ +-----------+ +-----------+
 | DS18B20x3 | | DHT22      | | ADXL345    | | Door reed  | | Tamper     |
 | (temp)    | | (humidity) | | (shock)     | | switch      | | switch      |
 +-----+-----+ +-----+-----+ +-----+-----+ +-----+-----+ +-----+-----+
       |             |             |             |             |
       +------+------+------+------+------+------+------+------+
                     v
           +-----------------------+
           |  Mega fusion + logic     |
           |  - excursion detection    |
           |  - closed-loop cooling     |
           +------------+---------------+
                         v
              +--------------------+
              | SD buffer (store-    |
              | and-forward queue)    |
              +----------+-----------+
                          v
              +--------------------+
              | SIM800L (AT cmds)    |
              | HTTP POST + token      |
              +----------+-----------+
                          v
              +--------------------+
              | Cloud ingest endpoint |
              +--------------------+
```

## Data Flow

1. **Sense** — every reading cycle, read all 3 DS18B20 probes, the DHT22, the ADXL345 (for a shock-magnitude peak), and both switches.
2. **Detect** — an excursion (temperature out of range, shock above threshold, door open, or tamper trip) is logged as an event row immediately, in addition to the regular periodic snapshot rows.
3. **Buffer** — every row (event or periodic) is appended to SD with a `synced=0` flag.
4. **Respond (local, always)** — a sustained temperature excursion independently engages the auxiliary cooling relay and buzzer, regardless of network state.
5. **Sync (when connected)** — `syncPendingRows()` walks the SD log for `synced=0` rows, sends each as an HTTP POST via the SIM800L, and flips the flag to `1` only on a confirmed success response.
6. **Retry** — rows that fail to sync stay `synced=0` and are retried on the next sync pass, in original order, giving store-and-forward reliability across connectivity gaps.

## Component Roles

- **DS18B20 x3 + DHT22 + ADXL345** — the sensor-fusion layer (5 distinct sensors), covering thermal, humidity, and mechanical-shock dimensions of the shipment.
- **Door/tamper switches** — the security-event layer.
- **SD** — the store-and-forward buffer, the project's core reliability trait.
- **SIM800L** — the sole network path; deliberately isolated behind a simple AT-command HTTP flow so the rest of the firmware doesn't need to know about cellular specifics.
- **Auxiliary cooling relay** — the closed-loop response to a confirmed thermal excursion.
