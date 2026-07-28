# Architecture — Six-Axis Robotic Arm with Inverse Kinematics

## System Diagram

```
 +----------------+
 |  Serial host   |
 |  (MOVE x y z)  |
 +--------+-------+
          v
 +------------------------+
 |   Command queue (Uno)   |
 +-----------+-------------+
             v
 +------------------------------+
 |  solveIK(x,y,z) -> 6 angles  |
 +---------------+---------------+
                 v
 +-------------------------------------+
 |  Trajectory interpolator (per-joint)|
 +---+------+------+------+------+-----+
     v      v      v      v      v     v
   base  shoulder elbow  wristP wristR grip
                    ^
                    | (angle feedback)
             +--------------+
             | Elbow pot A0 |
             +--------------+
                    v
           +--------------------+
           | Obstruction check  |
           +--------------------+
```

## Data Flow

1. **Input** — `MOVE x y z speed` lines are parsed and pushed onto a small FIFO queue.
2. **Solve** — `solveIK()` computes base rotation from `atan2(y,x)`, then shoulder/elbow angles from the law of cosines against the planar reach `r = sqrt(x^2+y^2)` and height `z`.
3. **Interpolate** — `updateServos()` steps each joint's commanded angle toward the solved target once per loop at the requested deg/s rate, avoiding a snap.
4. **Feedback** — `checkElbowFeedback()` reads A0 every loop and compares the mapped angle against the commanded elbow angle; if the error exceeds a threshold for more than 300ms, an obstruction fault is raised.
5. **Fault handling** — on fault, all further queue dispatch stops and servos hold their last position until `RESET` is received.
6. **Persistence** — `SAVE n`/`RECALL n` read/write a 6-angle pose struct to EEPROM slot `n`.

## Component Roles

- **Serial queue** — buffers waypoints so a host can queue ahead without waiting for each move.
- **IK solver** — the arm's only path from Cartesian targets to joint space.
- **Trajectory interpolator** — the arm's only path from joint targets to smooth physical motion.
- **Elbow feedback pot** — the arm's sole closed-loop sensor, used purely for stall/obstruction detection rather than position correction.
- **EEPROM** — durable storage for named poses across power cycles.
