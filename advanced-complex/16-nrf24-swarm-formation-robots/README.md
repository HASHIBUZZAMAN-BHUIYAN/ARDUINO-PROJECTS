# nRF24 Swarm Formation Robots

A leader robot broadcasts its dead-reckoned pose over nRF24L01 to two follower robots, which each run closed-loop ultrasonic distance-holding control to maintain a formation position behind/beside the leader — a best-effort wireless mesh prioritizing low latency over guaranteed delivery.

## Board(s) & Roles

- **Arduino Uno ("Leader")** — drives a preset waypoint script using wheel-encoder dead reckoning for its own pose, and broadcasts that pose over nRF24L01 continuously.
- **2x Arduino Nano ("Follower A", "Follower B")** — each receives the leader's broadcast pose, uses its own ultrasonic sensor to measure actual distance to the leader, and runs closed-loop PID drive control to hold its assigned formation offset.

## Components

| Part | Qty |
|---|---|
| Arduino Uno (leader) | 1 |
| Arduino Nano (follower) | 2 |
| nRF24L01+ radio module | 3 |
| Quadrature wheel encoder | 2 (leader only) |
| HC-SR04 ultrasonic sensor (distance to leader) | 2 (one per follower) |
| Dual motor driver (e.g. L298N) | 3 |
| DC gear motor + wheel + chassis | 3 sets |
| Jumper wires | ~30 |

## Architecture

The leader continuously estimates its own (x, y, heading) from wheel-encoder ticks and broadcasts it on a shared nRF24L01 pipe every 100ms — a best-effort broadcast with no acknowledgment required, since followers only ever care about the most recent pose and a dropped packet is harmless (a stale pose one cycle old is a fine approximation at these speeds). Each follower separately measures its actual distance to the leader via its own ultrasonic sensor and runs closed-loop PID drive control to correct toward its assigned target follow-distance, while a lightweight status pipe lets followers report their tracking error and battery back to the leader for monitoring. See [architecture.md](architecture.md) for the full diagram and data-flow walkthrough.

## Wiring

### Leader (Uno)

| Arduino Pin | Component | Notes |
|---|---|---|
| D9 (CE), D10 (CSN) | nRF24L01 | hardware SPI (D11-D13) |
| D2 (INT), A0 | Left encoder A (interrupt), B (polled) | |
| D3 (INT), A1 | Right encoder A (interrupt), B (polled) | |
| D4, D5, D6, D7 | L298N IN1-IN4 | |

### Each Follower (Nano)

| Arduino Pin | Component | Notes |
|---|---|---|
| D9 (CE), D10 (CSN) | nRF24L01 | hardware SPI |
| D6 (trig), D7 (echo) | HC-SR04 (distance to leader) | |
| D2, D3, D4, D5 | L298N IN1-IN4 | |

See [wiring/wiring.md](wiring/wiring.md) for the full ASCII diagram.

## Networking & Protocol

nRF24L01 (`RF24` library), two logical pipes:

- **Broadcast pipe** (leader → both followers, same address, no ack required): `{x_i16, y_i16, heading_i16, seq}` sent every 100ms — best-effort; a follower simply keeps driving toward its last-received pose if a packet is missed.
- **Status pipe** (each follower → leader, distinct addresses): `{followerId, distanceErrorCm_i16, batteryMv_u16}` sent at 2Hz for monitoring only, not used in the followers' own control loop.

## Setup & Deployment

1. Wire the leader's encoders, motor driver, and nRF24L01; wire each follower's ultrasonic sensor, motor driver, and nRF24L01.
2. Install `RF24` (by TMRh20) on all 3 boards (see `libraries.txt`).
3. Flash `src/follower/follower.ino` to both Nanos, setting each one's `MY_FOLLOWER_ID` and `TARGET_DISTANCE_CM` constants (e.g. Follower A trails at 40cm, Follower B at 60cm for a staggered line formation).
4. Flash `src/leader/leader.ino` to the Uno.
5. Power all 3 robots with the leader positioned in front. Confirm both followers begin driving to close on their target distance within a few seconds.
6. Drive the leader manually (or let its scripted waypoints run) and confirm both followers track it, maintaining their distinct target distances.

## Known Limitations & Path to Production

- Distance-holding alone (no lateral/bearing sensing) means followers can drift sideways relative to the leader over a long path — a production formation-following system would add a bearing sensor (e.g. a second ultrasonic pair, or vision) for full 2D relative positioning.
- Best-effort broadcast means a follower briefly "coasts" on stale data if several consecutive packets are dropped; a production system would add a timeout that stops the follower if the leader's broadcast goes silent for too long (similar to the heartbeat pattern used in this repo's `advanced/04` project).
- No collision avoidance between the two followers themselves.

## Extension Ideas

- Add a stale-broadcast timeout that halts a follower if it hasn't heard from the leader in over 1 second.
- Add a second ultrasonic sensor per follower for basic bearing estimation, enabling true 2D formation-holding instead of distance-only.
- Scale to a 3rd follower by adding a new status-pipe address — the broadcast pipe already supports any number of listeners with no leader-side changes.
