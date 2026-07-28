# Joystick Robot Arm

A 4-servo robot arm (base rotate, shoulder, elbow, gripper) is driven live by a joystick + two extra buttons, with a third button that records the current pose into an EEPROM-backed sequence for one-touch playback.

## Difficulty & Board

**Tier:** Intermediate
**Board:** Arduino Mega

Reasoning: 4 servos plus a joystick (2 analog axes + button) plus 2 extra control buttons (gripper open/close) plus a record/playback button is 8+ I/O lines with room to grow — comfortably possible on an Uno, but chosen here for the Mega to keep the board rotation balanced across the repo and to leave slack for the natural next step of adding more servo joints.

## Components

| Part | Qty |
|---|---|
| Arduino Mega 2560 | 1 |
| SG90 micro servo (base rotation) | 1 |
| SG90 micro servo (shoulder) | 1 |
| SG90 micro servo (elbow) | 1 |
| SG90 micro servo (gripper) | 1 |
| 2-axis analog joystick module (with button) | 1 |
| Pushbutton (record pose) | 1 |
| Pushbutton (play sequence) | 1 |
| 4-DOF arm chassis kit | 1 |
| External 5V/2A+ servo power supply | 1 |
| Jumper wires | ~14 |

## Wiring

| Arduino Pin | Component | Notes |
|---|---|---|
| A0 | Joystick VRx | base rotate |
| A1 | Joystick VRy | shoulder |
| D22 | Joystick SW | toggles gripper open/close |
| D23 | Elbow-up button | raises elbow while held |
| D24 | Elbow-down button | lowers elbow while held |
| D25 | Record-pose button | saves current pose to the sequence |
| D26 | Play-sequence button | replays all saved poses in order |
| D9 | Base servo signal | |
| D10 | Shoulder servo signal | |
| D11 | Elbow servo signal | |
| D12 | Gripper servo signal | |
| External 5V | All 4 servo VCC | **not** powered from the Mega's 5V pin |

## How It Works

Base and shoulder are driven directly by the joystick's two axes (speed-proportional movement, same technique as the pan-tilt project). Elbow uses two separate up/down buttons instead of an axis (arms typically only need coarse elbow control, and it frees the joystick's button for gripper toggle). The gripper servo only has two useful positions (open/closed), so its button is a simple toggle rather than a continuous control.

The **record/playback** feature is what elevates this beyond simple teleoperation: pressing the record button appends the arm's current 4 servo angles as one "waypoint" into a small in-memory array (up to `MAX_WAYPOINTS`), which is also written to `EEPROM` so a saved sequence survives a power cycle. Pressing play steps through every saved waypoint in order, smoothly interpolating each servo from its current angle to the next waypoint's angle over a fixed duration, effectively turning manual joystick moves into a repeatable programmed motion — the same core idea used in real industrial "teach pendant" robot arms.

## Setup & Flashing

1. Assemble the 4-DOF arm chassis and wire the joystick, buttons, and servos as above. Power all 4 servos from a dedicated external 5V supply (moving 4 servos at once can exceed what the Mega's onboard regulator/USB can safely provide) with grounds tied to the Mega.
2. No non-built-in libraries are required — `Servo` and `EEPROM` are bundled with the Arduino IDE.
3. Open `src/joystick_robot_arm.ino` in the Arduino IDE.
4. Select **Tools > Board > Arduino Mega or Mega 2560** and the correct COM port.
5. Upload, test manual joystick/button control first, then record 3-4 waypoints and press play to confirm smooth playback.

## Extensions

- Add a potentiometer-based wrist-rotation axis for a 5th degree of freedom.
- Add a "loop sequence" mode that replays the saved waypoints continuously (e.g. a simple pick-and-place demo).
- Control the arm remotely instead of with a local joystick, using the advanced-tier nRF24L01 wireless link pattern.
