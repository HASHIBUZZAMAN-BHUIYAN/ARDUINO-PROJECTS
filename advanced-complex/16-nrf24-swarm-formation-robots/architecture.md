# Architecture — nRF24 Swarm Formation Robots

## System Diagram

```
 +------------------------+
 |  Leader (Uno)           |
 |  - encoder dead reckoning|
 |  - broadcasts pose        |
 +------------+-------------+
              | nRF24L01 broadcast pipe (best-effort, 100ms)
     +--------+--------+
     v                 v
+----------+       +----------+
|Follower A|       |Follower B|
|(Nano)    |       |(Nano)    |
|- ultra-  |       |- ultra-  |
|  sonic   |       |  sonic   |
|- PID hold|       |- PID hold|
+----+-----+       +----+-----+
     |                   |
     +---- status pipe --+---> back to leader (monitoring only)
```

## Data Flow

1. **Estimate (leader)** — wheel-encoder ticks are integrated every control tick into a running (x, y, heading) pose.
2. **Broadcast (leader)** — the pose is sent as a small struct on a shared pipe every 100ms with no ack requested — a dropped packet is simply not retried.
3. **Receive (followers)** — each follower reads whatever pose arrived most recently; if none arrived this cycle, it keeps using the last one (staleness is tolerated by design).
4. **Sense (followers)** — each follower's own HC-SR04 measures actual distance to the leader independent of the broadcast pose.
5. **Control (followers)** — a PID loop compares measured distance to the follower's own `TARGET_DISTANCE_CM` and adjusts drive motor speed to close the gap.
6. **Report (followers)** — a lower-rate status packet (distance error, battery) goes back to the leader purely for monitoring, not control.

## Component Roles

- **Leader** — sole pose source; never listens for follower control input, keeping the system simple and one-directional for the control-relevant broadcast.
- **Followers** — each independently closes its own control loop from local ultrasonic sensing; the broadcast pose is a "where roughly is the leader" hint, not a control input for the followers' distance PID (which uses only local ultrasonic feedback).
- **nRF24L01 modules** — the wireless layer; broadcast pipe is fire-and-forget, status pipe is low-rate and non-critical.
