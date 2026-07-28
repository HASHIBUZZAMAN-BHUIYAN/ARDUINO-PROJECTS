# Servo Pan-Tilt Camera Mount

A two-axis joystick smoothly drives a two-servo pan-tilt bracket (the kind small cameras or laser pointers mount on), with the joystick's button re-centering both axes instantly.

## Difficulty & Board

**Tier:** Intermediate
**Board:** Arduino Nano

Reasoning: two servos plus a joystick module (2 analog axes + button) is a small, well-contained pin count for a compact desk/tripod-mounted gadget — a good fit for the Nano rather than a full-size Uno.

## Components

| Part | Qty |
|---|---|
| Arduino Nano | 1 |
| SG90 micro servo (pan axis) | 1 |
| SG90 micro servo (tilt axis) | 1 |
| 2-axis analog joystick module (with button) | 1 |
| Pan-tilt bracket kit | 1 |
| Breadboard | 1 |
| Jumper wires | 7 |

## Wiring

| Arduino Pin | Component | Notes |
|---|---|---|
| 5V | Joystick VCC, both servo VCC | |
| GND | Joystick GND, both servo GND | shared ground |
| A0 | Joystick VRx | pan axis input |
| A1 | Joystick VRy | tilt axis input |
| D8 | Joystick SW (button) | re-center both servos |
| D9 | Pan servo signal | |
| D10 | Tilt servo signal | |

## How It Works

Each joystick axis centers around roughly 512 (out of 0-1023) at rest. The sketch treats a deadzone around that center as "no input" (to prevent drift from a joystick that doesn't return to *exactly* center), and outside the deadzone, maps the axis reading to a **speed**, not a direct angle — pushing the stick further from center moves the servo faster, rather than snapping straight to a target position. This gives smooth, analog-feeling pan/tilt control similar to a real camera gimbal joystick, rather than the twitchy jumps you'd get by mapping the stick position directly to servo angle every loop.

Both servo angles are tracked in software as floats and clamped to a safe 0-180 range (further trimmed to protect the pan-tilt bracket's mechanical limits) each loop, then written to the servos. Pressing the joystick button snaps both axes back to their 90-degree center position instantly.

## Setup & Flashing

1. Assemble the pan-tilt bracket with both servos, then wire the joystick and servos as above.
2. Open `src/pan_tilt_control.ino` in the Arduino IDE. The **Servo** library is bundled with the IDE — no separate install needed.
3. Select **Tools > Board > Arduino Nano** (correct processor/bootloader for your clone) and the correct COM port.
4. Upload, then test that pushing the joystick smoothly pans/tilts and that releasing it back to center stops movement (rather than continuing to drift).
5. Press the joystick button to confirm both servos re-center.

## Extensions

- Mount a small laser pointer or phone-camera clip on the bracket for an actual pan-tilt camera rig.
- Add a "patrol mode" button that sweeps the bracket through a preset pattern automatically (using the same technique as the intermediate joystick robot arm's saved positions).
- Combine with the basic ultrasonic sensor project to auto-track the closest object.
