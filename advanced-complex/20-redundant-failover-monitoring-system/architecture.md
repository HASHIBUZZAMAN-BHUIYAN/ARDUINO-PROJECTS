# Architecture — Redundant Dual-MCU Failover Monitoring System

## System Diagram

```
   Shared sensors (temp, gas, leak) -- wired to BOTH boards in parallel
        |                                          |
        v                                          v
 +----------------+   heartbeat (UART)     +----------------+
 |  Primary (Uno)  |<---------------------->|  Standby (Uno)  |
 |  ACTIVE by       |                        |  LISTENING by   |
 |  default          |                        |  default         |
 +--------+---------+                        +--------+---------+
          |                                            |
          v  (only the ACTIVE unit drives these)        v
   Shared actuators (siren relay, valve relay) -- wired to BOTH boards
```

## Data Flow

1. **Sense (active unit only drives logic from its own readings)** — the active board reads all 3 sensors every control tick and evaluates the alarm logic.
2. **Actuate (active unit only)** — the active board's actuator pins are `OUTPUT` and directly reflect the alarm logic's result; the standby unit's same-numbered pins are `INPUT` (high-impedance) so they never contest the shared lines.
3. **Heartbeat (active unit only)** — the active board transmits `HB,<seq>,<state>` every 250ms.
4. **Watch (standby unit only)** — the standby board tracks time since the last heartbeat it received.
5. **Promote** — if that gap exceeds 1000ms, the standby board flips its actuator pins to `OUTPUT`, begins sampling sensors and driving actuators itself, and starts sending its own heartbeat.
6. **Yield** — if the original primary board comes back and hears a heartbeat it did not itself send, it demotes: actuator pins go back to `INPUT`, and it resumes listening instead of driving.

## Component Roles

- **Primary / Standby** — symmetric hardware, asymmetric default role; the same firmware pattern (active vs. listening state machine) governs both, just with opposite starting states.
- **Shared sensors** — a single physical sensing layer read redundantly by whichever board is currently active.
- **Shared actuators** — a single physical actuation layer whose safety property (never double-driven) depends entirely on the active/standby state machine's pin-direction discipline.
- **UART heartbeat** — the sole signal that arbitrates which board is allowed to drive actuators at any moment.
