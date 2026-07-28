# Architecture — Conveyor Sorting Dual-Board Automation

## System Diagram

```
 +----------------+
 |  TCS3200        |
 |  color sensor   |
 +--------+--------+
          v
 +------------------------+     I2C     +--------------------------+
 |  Uno "Sorter" (0x10)    |<----------->|  Mega "Line Controller"    |
 |  - detects color         |            |  - PID belt speed          |
 |  - swings diverter servo |            |  - IR break-beam feedback  |
 +------------------------+             |  - object counting          |
                                          +------------+--------------+
                                                       v
                                          +------------------------+
                                          |  Conveyor motor (PWM)    |
                                          +------------------------+
```

## Data Flow

1. **Detect** — the sorter continuously reads the TCS3200 and classifies the dominant color.
2. **Speed control** — the line controller reads the IR break-beam's pulse rate, computes a PID correction against the target belt speed, and adjusts the motor's PWM duty cycle.
3. **Count** — every break-beam interruption also increments an item counter on the line controller.
4. **Query** — as an item approaches the sorter's known position (timed from the break-beam event plus belt speed), the line controller requests the sorter's last-detected color over I2C.
5. **Command** — the line controller immediately sends back a lane command over I2C.
6. **Actuate** — the sorter swings its diverter servo to the commanded lane.

## Component Roles

- **TCS3200 + sorter (Uno)** — the sensing and gate-actuation role, kept as a self-contained slave.
- **IR break-beam + line controller (Mega)** — the timing/speed/counting role, kept as the bus master and owner of the conveyor's closed loop.
- **I2C link** — the sole coordination point between color detection and diverter action.
