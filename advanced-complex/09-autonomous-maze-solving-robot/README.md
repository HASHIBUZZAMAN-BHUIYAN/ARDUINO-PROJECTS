# Autonomous Maze-Solving Robot

A Mega fuses an IR wall-sensor array, a front ultrasonic sensor, wheel encoders, and an IMU into a dead-reckoned grid position, explores an unknown maze with a flood-fill algorithm, stores the solved maze and optimal path in EEPROM, then replays the optimal path at speed on a second run — a classic micromouse "explore then speed-run" pattern.

## Board

**Arduino Mega 2560** — 2 drive motors, 3 IR wall sensors, 1 ultrasonic sensor, 2 quadrature encoders (4 pins), and an I2C IMU together exceed what's comfortable on an Uno, and the Mega's extra hardware interrupt pins accommodate both encoders cleanly.

## Components

| Part | Qty |
|---|---|
| Arduino Mega 2560 | 1 |
| IR reflectance/wall-distance sensor (front-left, front-right, side) | 3 |
| HC-SR04 ultrasonic sensor (front, long-range) | 1 |
| Quadrature wheel encoder | 2 |
| MPU6050 IMU (I2C, for heading) | 1 |
| Dual motor driver (e.g. L298N) | 1 |
| DC gear motor + wheel | 2 |
| Chassis + caster wheel | 1 |
| Jumper wires | ~25 |

## Architecture

Wall sensors, the ultrasonic sensor, wheel encoders, and the IMU are fused each control tick into a per-cell "walls present" reading and a continuously-updated (x, y, heading) pose estimate. During the first (explore) run, a flood-fill algorithm updates a distance-to-goal map in RAM as new walls are discovered and always drives toward the lowest-distance neighbor; once the goal is reached, the discovered maze and the flood-fill-computed optimal path are written to EEPROM. On a second (speed) run, the stored optimal path is replayed directly, with closed-loop PID motor control keeping the robot driving straight between cells using fused encoder+IMU heading feedback. See [architecture.md](architecture.md) for the full diagram and data-flow walkthrough.

## Wiring

| Arduino Mega Pin | Component | Notes |
|---|---|---|
| A0, A1, A2 | IR sensors (front-left, front-right, side) | analog distance output |
| D6 (trig), D7 (echo) | HC-SR04 ultrasonic | |
| D2 (INT), D3 | Left encoder A/B | D2 on hardware interrupt |
| D18 (INT), D19 | Right encoder A/B | D18 on hardware interrupt |
| 20/21 (SDA/SCL) | MPU6050 | |
| D8, D9, D10, D11 | L298N IN1-IN4 | motor direction/speed |

See [wiring/wiring.md](wiring/wiring.md) for the full ASCII diagram.

## Setup & Deployment

1. Wire all sensors, encoders, IMU, and the motor driver as above; verify encoder direction matches motor direction (swap A/B leads if the reported distance goes negative while driving forward).
2. Install `Adafruit MPU6050` + dependencies (see `libraries.txt`).
3. Open `src/maze_solver.ino`, calibrate `IR_WALL_THRESHOLD` and `TICKS_PER_CELL` for your physical maze cell size and sensor mounting.
4. Upload to the Mega, place the robot at the maze start, and power on in **explore mode** (default on first boot / after `EEPROM` is cleared).
5. Let the robot fully explore and reach the goal; confirm it prints "Maze solved, path stored" and the maze data appears sensible on Serial.
6. Reset the robot to the start cell and power-cycle; it will detect a valid stored maze and automatically run in **speed mode**, following the stored optimal path.

## Known Limitations & Path to Production

- Dead reckoning (encoders + IMU) drifts over a long maze without any absolute position correction — a production micromouse adds wall-following corrections opportunistically as it drives.
- Flood-fill re-plans crudely (full recompute each new wall discovery) rather than an incremental algorithm — fine at this maze scale, would need optimization for larger mazes.
- No collision/bump recovery — a wall discovered by physically bumping it (not the IR sensors) simply stalls the robot.

## Extension Ideas

- Add wall-following-based heading correction opportunistically during exploration to reduce drift.
- Support re-exploration if a partially-changed maze is detected (mismatch between stored and sensed walls).
- Log each run's time-to-goal to SD to track speed-run improvement over multiple attempts.
