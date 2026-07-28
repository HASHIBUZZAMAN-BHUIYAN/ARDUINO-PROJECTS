# Obstacle-Avoiding Robot Car

A two-wheel-drive robot car drives forward until its ultrasonic sensor detects an obstacle, then it stops, backs up, and turns to find a clearer path.

## Difficulty & Board

**Tier:** Intermediate
**Board:** Arduino Mega

Reasoning: an L298N motor driver takes 4 control pins plus 2 PWM speed pins, the ultrasonic sensor takes 2 more, and a servo to sweep the sensor for look-left/look-right checks adds one more PWM pin — that's 9 I/O lines before you've added anything else. It comfortably fits on an Uno, but this project is a natural stepping stone toward more sensors (line-following, bumper switches) later, so it's built on the Mega to leave headroom and to keep the board rotation balanced across the repo.

## Components

| Part | Qty |
|---|---|
| Arduino Mega 2560 | 1 |
| L298N dual H-bridge motor driver module | 1 |
| TT gear motors + wheels | 2 |
| Caster wheel (for balance) | 1 |
| HC-SR04 ultrasonic sensor | 1 |
| SG90 micro servo (sensor pan) | 1 |
| Robot chassis | 1 |
| 4x AA battery pack (motor power) | 1 |
| Jumper wires | ~14 |

## Wiring

| Arduino Pin | Component | Notes |
|---|---|---|
| D22 | L298N IN1 | left motor direction |
| D23 | L298N IN2 | left motor direction |
| D5 | L298N ENA (PWM) | left motor speed |
| D24 | L298N IN3 | right motor direction |
| D25 | L298N IN4 | right motor direction |
| D6 | L298N ENB (PWM) | right motor speed |
| 5V | HC-SR04 VCC | |
| GND | HC-SR04 GND, L298N GND, battery pack GND | common ground across all power domains |
| D9 | HC-SR04 TRIG | |
| D10 | HC-SR04 ECHO | |
| D11 | Sensor-pan servo signal | sweeps ultrasonic sensor left/right to "look" |
| Battery + | L298N 12V input | motor power, separate from Mega's own supply |

## How It Works

The car drives forward continuously while polling the ultrasonic sensor. When an obstacle is detected within `STOP_DISTANCE_CM`:

1. Both motors stop.
2. The car backs up briefly to give itself room to turn.
3. The pan servo sweeps the ultrasonic sensor to look left, then right, taking a distance reading at each position.
4. The car turns (in place, one wheel forward/one reverse) toward whichever side had more clearance, then resumes driving forward.

Motor direction is set with two digital pins per motor (`IN1/IN2`, `IN3/IN4` — one HIGH/one LOW for forward, swapped for reverse, both LOW to coast) while speed is set separately via PWM on the driver's enable pins (`ENA`/`ENB`). Keeping direction and speed as two independent controls (rather than one combined function) is the key motor-driver concept this project teaches, since nearly every H-bridge module works this same way.

## Setup & Flashing

1. Assemble the chassis, mount both gear motors, the caster wheel, and the pan servo with the ultrasonic sensor on top of it.
2. Wire the L298N, ultrasonic sensor, and servo as above. Power the L298N's motor supply from the battery pack, **not** from the Mega's 5V pin — motors draw far more current than the onboard regulator can safely supply.
3. Install the **Servo** library if not already present (bundled with the Arduino IDE).
4. Open `src/obstacle_avoider.ino` in the Arduino IDE.
5. Select **Tools > Board > Arduino Mega or Mega 2560** and the correct COM port.
6. Upload while the car is off the ground (wheels spinning freely) to sanity-check motor directions before setting it down.

## Extensions

- Add a line-tracking IR sensor pair to switch between "avoid obstacles" and "follow line" modes.
- Add bump switches at the front as a mechanical backup in case the ultrasonic sensor misses a low or angled obstacle.
- Log turn decisions and distances over Serial/SD to analyze how it navigates a specific room layout.
