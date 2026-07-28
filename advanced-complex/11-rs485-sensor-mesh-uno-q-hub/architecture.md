# Architecture — RS-485 Sensor Mesh with Uno Q Hub

## System Diagram

```
 +---------------+  +---------------+  +---------------+
 | Node 1        |  | Node 2        |  | Node 3        |
 | Climate       |  | Air Quality   |  | Occupancy     |
 | (Uno + MAX485)|  | (Uno + MAX485)|  | (Uno + MAX485)|
 +-------+-------+  +-------+-------+  +-------+-------+
         |                  |                  |
         +--------+---------+---------+--------+
                  |  RS-485 bus (A/B + GND, daisy-chained) |
                  v
         +--------------------------+
         |  Uno Q Hub (MAX485 master)|
         |  - poll cycle (1,2,3)      |
         |  - SD logging (RTC ts)     |
         +------------+---------------+
                      v
              +------------------+
              | W5500 HTTP API    |
              +--------+----------+
                        v
              +------------------+
              | dashboard/*.html  |
              +------------------+
```

## Data Flow

1. **Sense (each node)** — every loop, a field node reads its two sensors and waits for a matching `POLL <addr>` on the RS-485 bus.
2. **Poll (hub)** — the hub cycles through addresses 1, 2, 3 every 10 seconds, asserting DE/RE to transmit `POLL <addr>\n`, then releasing DE/RE to listen.
3. **Respond (node)** — the addressed node asserts its own DE/RE, sends `D,<addr>,<v1>,<v2>,<seq>,<chk>\n`, and releases DE/RE back to listen mode.
4. **Validate (hub)** — the hub checks the checksum; on success it updates that node's in-RAM status and appends a timestamped row to SD; on failure/timeout it retries once, then marks the node offline for this cycle.
5. **Serve** — the W5500 HTTP server answers `/api/nodes` with the latest status array.
6. **Display** — the dashboard polls `/api/nodes` and renders a per-node status table.

## Component Roles

- **Field nodes** — independent sensor sources, each responsible only for answering its own address's poll.
- **MAX485 transceivers** — convert each board's UART to the shared differential RS-485 bus; DE/RE gating is what prevents multiple boards from transmitting at once.
- **Uno Q hub** — the bus master, owner of the poll cycle, SD log, and HTTP server.
- **RTC** — authoritative timestamp source for the SD log.
- **SD + W5500** — durable history and LAN-facing live view, respectively.
