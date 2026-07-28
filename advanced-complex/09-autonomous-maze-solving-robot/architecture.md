# Architecture — Autonomous Maze-Solving Robot

## System Diagram

```
 +-----------+  +-----------+  +-----------+  +-----------+  +-----------+
 | IR sensors|  |Ultrasonic |  |Encoders   |  | MPU6050   |  |           |
 | (x3)      |  |(front)    |  |(L+R)      |  | (heading) |  |           |
 +-----+-----+  +-----+-----+  +-----+-----+  +-----+-----+  +-----------+
       |              |              |              |
       +------+-------+------+-------+------+-------+
                     v
           +-----------------------+
           |  Sensor fusion         |
           |  - wall map per cell    |
           |  - (x,y,heading) pose   |
           +-----------+-------------+
                       v
        +-------------------------------+
        | Explore mode: flood-fill       |<--- (first run)
        | Speed mode: replay stored path |<--- (subsequent runs)
        +---------------+-----------------+
                        v
              +--------------------+
              | Closed-loop PID     |
              | motor control       |
              +--------------------+
                        v
              +--------------------+       +------------------+
              | Drive motors        |       | EEPROM: maze map  |
              +--------------------+       | + optimal path     |
                                            +------------------+
```

## Data Flow

1. **Sense** — every control tick, read all 3 IR sensors, the ultrasonic sensor, both encoders' tick counts, and the IMU's yaw.
2. **Fuse** — `updatePose()` integrates encoder ticks and IMU yaw into a running (x, y, heading) estimate snapped to the nearest maze cell; `updateWallMap()` marks walls around the current cell from sensor readings above threshold.
3. **Decide (explore)** — `floodFill()` recomputes each cell's distance-to-goal whenever a new wall is discovered; the robot always drives toward the accessible neighbor with the lowest distance.
4. **Decide (speed run)** — once a full solution exists, `replayPath()` simply steps through the stored cell sequence.
5. **Control** — `driveToCell()` uses closed-loop PID (fused encoder+IMU heading error) to drive straight and turn accurately between cells regardless of which decision mode is active.
6. **Persist** — on reaching the goal during exploration, `saveMazeToEEPROM()` writes the wall map and computed path so the next boot can go straight to speed mode.

## Component Roles

- **IR sensors** — local wall presence, the raw input to the wall map.
- **Ultrasonic sensor** — longer-range front clearance check, catches walls the short-range IR array might miss early.
- **Encoders + IMU** — the position/heading fusion pair the whole navigation stack depends on.
- **Mega** — owns fusion, mapping, decision-making, and closed-loop control.
- **EEPROM** — the memory that turns "explore" into "replay," the core of the trait.
