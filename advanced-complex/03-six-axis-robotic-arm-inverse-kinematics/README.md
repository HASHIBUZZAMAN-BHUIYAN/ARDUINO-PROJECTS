# Six-Axis Robotic Arm with Inverse Kinematics

A 6-servo arm computes joint angles from a target (x,y,z) end-effector position using geometric inverse kinematics, executes a serial-queued list of waypoints with smooth trajectory interpolation, and uses a feedback potentiometer on the elbow joint to detect stalls/obstructions in closed loop.

## Board

**Arduino Uno** — 6 servos plus one feedback potentiometer fit comfortably within the Uno's pin count; this project's complexity is in the math and control logic, not pin volume, so the smaller board is the appropriate (and cheaper) choice.

## Components

| Part | Qty |
|---|---|
| Arduino Uno | 1 |
| Hobby servo (base, shoulder, wrist-pitch, wrist-roll, gripper) | 5 |
| Hobby servo with free-spinning feedback pot (elbow) | 1 |
| 10 kΩ potentiometer (elbow angle feedback, geared to elbow joint) | 1 |
| 5V/5A external servo power supply | 1 |
| Jumper wires | ~20 |

## Architecture

Target coordinates arrive over serial as queued `MOVE x y z speed` commands; each is converted by `solveIK()` into 6 joint angles via the law of cosines for the shoulder/elbow 2-link geometry plus direct mapping for base rotation and wrist orientation, then smoothly interpolated toward over time rather than snapped. The elbow's geared feedback potentiometer is read continuously and compared against the commanded angle; a persistent large error trips an "obstruction" fault that halts the queue. See [architecture.md](architecture.md) for the full diagram and data-flow walkthrough.

## Wiring

| Arduino Pin | Component | Notes |
|---|---|---|
| D3 | Base servo signal | |
| D5 | Shoulder servo signal | |
| D6 | Elbow servo signal | |
| D9 | Wrist-pitch servo signal | |
| D10 | Wrist-roll servo signal | |
| D11 | Gripper servo signal | |
| A0 | Elbow feedback potentiometer wiper | geared 1:1 to elbow joint |
| VIN (servo rail) | External 5V/5A supply | **not** the Uno's 5V pin — 6 servos can exceed what the USB/onboard regulator supplies |

See [wiring/wiring.md](wiring/wiring.md) for the full ASCII diagram.

## Networking & Protocol

Serial command queue at 9600 baud, one command per line:

- `MOVE x y z speed` — queue a target end-effector position (mm) and interpolation speed (deg/s); the arm services queued moves one at a time, only starting the next once the current one's interpolation completes.
- `GRIP open|close` — queue a gripper action.
- `SAVE n` / `RECALL n` — store/recall one of 8 EEPROM pose slots.
- On obstruction fault, the arm halts the remaining queue and prints `FAULT OBSTRUCTION` until a `RESET` command is received.

## Setup & Deployment

1. Wire all 6 servos and the elbow feedback pot as above; power servos from the external 5V/5A supply with a common ground to the Uno.
2. Install `Servo` and `EEPROM` (both built-in — see `libraries.txt`).
3. Open `src/robotic_arm.ino`, adjust `L1`/`L2` link-length constants (mm) to match your physical arm.
4. Upload to the Uno; open Serial Monitor at 9600 baud.
5. Send `MOVE 120 0 80 30` and confirm all joints move smoothly to reach the target without snapping.
6. Manually hold the elbow joint against motion briefly and confirm `FAULT OBSTRUCTION` is reported, then send `RESET` to clear it.

## Known Limitations & Path to Production

- Only the elbow joint has position feedback; the other 5 joints are open-loop (trust the servo's internal position). Production-grade would add feedback (e.g. magnetic encoders) to every joint.
- IK solver assumes a simple 2-link planar shoulder/elbow with independent base rotation and wrist orientation — it does not solve full 6-DOF closed-form kinematics for arbitrary wrist orientations.
- No collision/self-intersection checking before executing a queued move.

## Extension Ideas

- Add feedback potentiometers to the shoulder and base joints for full closed-loop control.
- Add a simple trajectory blending mode so consecutive queued moves don't fully stop between waypoints.
- Expose the same `MOVE`/`GRIP` protocol over a Bluetooth serial module for wireless teleoperation.
