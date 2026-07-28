# ARDUINO-PROJECTS

A collection of 50 varied Arduino-family hardware projects, spanning home automation, robotics, IoT/telemetry, sensors & monitoring, security, audio/visual, environmental, and wearable builds. Projects are split into four difficulty tiers and rotate across four boards: **Uno**, **Nano**, **Uno Q**, and **Mega**.

Licensed under the [PolyForm Strict License 1.0.0](LICENSE) — restricted, non-redistributable.

## Repository Structure

```
basic/               8 beginner-friendly, single sensor/actuator projects
intermediate/        10 projects combining 2-4 components with some logic/connectivity
advanced/            7 multi-component systems: networking, data logging, automation, multi-board
advanced-complex/    25 portfolio/production-grade projects: sensor fusion, control loops,
                     multi-board systems, networking stacks, and flagship builds
```

Each project folder contains a `README.md` (parts list, wiring table, how it works, setup, extensions), a `src/` folder with the runnable `.ino` sketch(es), a `wiring/` folder with a pin-diagram note, and a `libraries.txt` listing any required external Arduino libraries (omitted when only built-in libraries are used). `advanced-complex/` projects additionally include an `architecture.md` (system diagram + data flow) and, where applicable, a `dashboard/` folder with a self-contained web UI.

## Project Index

### Basic (8)

| Name | Board | Tier | Description | Link |
|---|---|---|---|---|
| Morse Code LED Beacon | Uno | Basic | Blinks a fixed message on one LED using real Morse code timing ratios. | [basic/01-morse-code-led-beacon](basic/01-morse-code-led-beacon) |
| Ultrasonic Parking Sensor | Uno | Basic | HC-SR04 distance sensor beeps faster as an obstacle gets closer. | [basic/02-ultrasonic-parking-sensor](basic/02-ultrasonic-parking-sensor) |
| Light-Activated Night Light | Nano | Basic | LDR-driven LED that fades on at dusk and off at dawn, with hysteresis. | [basic/03-light-activated-night-light](basic/03-light-activated-night-light) |
| PIR Motion Alarm | Uno | Basic | PIR sensor sounds a buzzer and lights an LED whenever motion is detected. | [basic/04-pir-motion-alarm](basic/04-pir-motion-alarm) |
| Soil Moisture Plant Alert | Uno Q | Basic | Capacitive soil probe chirps once when a potted plant needs watering. | [basic/05-soil-moisture-plant-alert](basic/05-soil-moisture-plant-alert) |
| Sound-Activated Clap Switch | Nano | Basic | Classic "Clapper"-style double-clap toggles a relay-driven lamp. | [basic/06-sound-activated-clap-switch](basic/06-sound-activated-clap-switch) |
| Analog Servo Gauge | Nano | Basic | A potentiometer smoothly drives a servo needle across a printed dial. | [basic/07-analog-servo-gauge](basic/07-analog-servo-gauge) |
| Digital Dice | Uno Q | Basic | Button press "rolls" a die shown in the classic 7-LED dot pattern. | [basic/08-digital-dice](basic/08-digital-dice) |

### Intermediate (10)

| Name | Board | Tier | Description | Link |
|---|---|---|---|---|
| DIY Weather Station | Uno | Intermediate | DHT22 + BMP280 on an LCD with a rolling pressure trend indicator. | [intermediate/01-diy-weather-station](intermediate/01-diy-weather-station) |
| Obstacle-Avoiding Robot Car | Mega | Intermediate | Ultrasonic + servo-pan "look both ways" robot car that steers around obstacles. | [intermediate/02-obstacle-avoiding-robot-car](intermediate/02-obstacle-avoiding-robot-car) |
| RFID Door Lock | Nano | Intermediate | MFRC522 card scan swings a servo bolt open for authorized cards. | [intermediate/03-rfid-door-lock](intermediate/03-rfid-door-lock) |
| LCD Menu System | Uno | Intermediate | Rotary-encoder-driven menu controlling 3 relays and an EEPROM-saved backlight level. | [intermediate/04-lcd-menu-system](intermediate/04-lcd-menu-system) |
| IR Remote Control Hub | Uno Q | Intermediate | Repurposes a generic IR remote to control a 4-channel relay "appliance" hub. | [intermediate/05-ir-remote-control-hub](intermediate/05-ir-remote-control-hub) |
| Servo Pan-Tilt Camera Mount | Nano | Intermediate | Joystick drives a smooth, speed-proportional 2-axis pan-tilt bracket. | [intermediate/06-servo-pan-tilt-camera-mount](intermediate/06-servo-pan-tilt-camera-mount) |
| Two-Zone Security Alarm | Uno | Intermediate | Keypad arm/disarm with exit/entry delays across a door and a PIR zone. | [intermediate/07-two-zone-security-alarm](intermediate/07-two-zone-security-alarm) |
| Joystick Robot Arm | Mega | Intermediate | 4-servo arm teleoperated by joystick, with EEPROM record/playback of poses. | [intermediate/08-joystick-robot-arm](intermediate/08-joystick-robot-arm) |
| Bluetooth Smart Lamp | Nano | Intermediate | HC-05 Bluetooth serial commands set an RGB strip's color, brightness, and effects. | [intermediate/09-bluetooth-smart-lamp](intermediate/09-bluetooth-smart-lamp) |
| Wearable Step Counter & Posture Coach | Nano | Intermediate | MPU6050 wearable counts steps and buzzes a vibration motor on sustained slouching. | [intermediate/10-wearable-step-counter-posture-coach](intermediate/10-wearable-step-counter-posture-coach) |

### Advanced (7)

| Name | Board | Tier | Description | Link |
|---|---|---|---|---|
| Mega Multi-Sensor Environmental Dashboard | Mega | Advanced | 5-sensor dashboard on a 20x4 LCD with RTC-timestamped SD card logging. | [advanced/01-mega-multisensor-dashboard](advanced/01-mega-multisensor-dashboard) |
| Stepper Motor CNC Mini Plotter | Mega | Advanced | 2-axis stepper pen plotter driven by a tiny G-code-like serial command language. | [advanced/02-cnc-mini-plotter](advanced/02-cnc-mini-plotter) |
| Ethernet Home Automation Server | Uno Q | Advanced | Hosts its own web page over wired Ethernet to toggle 4 relays from any browser. | [advanced/03-ethernet-home-automation-server](advanced/03-ethernet-home-automation-server) |
| Dual-Board Wireless Security System | Nano + Uno | Advanced | nRF24L01-linked remote sensor node and base station with heartbeat offline detection. | [advanced/04-dual-board-wireless-security-system](advanced/04-dual-board-wireless-security-system) |
| GPS Data Logging Tracker | Uno | Advanced | Logs a GPS track to SD, using an RTC as a backup clock before a fix is acquired. | [advanced/05-gps-data-logging-tracker](advanced/05-gps-data-logging-tracker) |
| RFID + Keypad Access Control Logger | Mega | Advanced | Two-factor (card + PIN) door control with a full timestamped SD audit trail. | [advanced/06-rfid-keypad-access-logger](advanced/06-rfid-keypad-access-logger) |
| ESP8266 WiFi Weather Telemetry | Uno Q | Advanced | DHT22 + BMP280 readings pushed over WiFi via an ESP8266 AT-command co-processor. | [advanced/07-esp8266-wifi-weather-telemetry](advanced/07-esp8266-wifi-weather-telemetry) |

### Advanced-Complex (25)

Portfolio/production-grade builds: heavy sensor fusion, on-device signal processing/ML, closed-loop control, multi-board coordination (RS-485, I2C multi-drop, UART command queues, nRF24 mesh), networking stacks (REST APIs, local dashboards, MQTT), persistent time-series storage with historical analytics, and (for the tier's 5 flagships) production-hardening like token auth, EEPROM/store-and-forward persistence, and watchdog recovery.

| Name | Board(s) | One-liner | Link |
|---|---|---|---|
| PID Greenhouse Climate Controller | Mega | Closed-loop PID heater/fan/mister control with SD logging and a local Ethernet dashboard. | [advanced-complex/01-pid-greenhouse-climate-controller](advanced-complex/01-pid-greenhouse-climate-controller) |
| G-Code CNC Mill Controller with Closed-Loop Homing | Mega | Parses G-code, drives 3 stepper axes, and homes against limit switches with a flow-controlled move queue. | [advanced-complex/02-gcode-cnc-mill-controller-with-homing](advanced-complex/02-gcode-cnc-mill-controller-with-homing) |
| Six-Axis Robotic Arm with Inverse Kinematics | Uno | Solves joint angles from (x,y,z) targets and queues waypoints with closed-loop obstruction detection. | [advanced-complex/03-six-axis-robotic-arm-inverse-kinematics](advanced-complex/03-six-axis-robotic-arm-inverse-kinematics) |
| Vibration FFT Predictive Maintenance Monitor | Uno | On-device FFT flags abnormal vibration signatures and trips a machine-isolation relay. | [advanced-complex/04-vibration-fft-predictive-maintenance-monitor](advanced-complex/04-vibration-fft-predictive-maintenance-monitor) |
| Solar MPPT Battery Management System | Uno Q | Perturb & Observe MPPT tracking, coulomb-counted state of charge, and a live Ethernet dashboard. | [advanced-complex/05-solar-mppt-battery-management-system](advanced-complex/05-solar-mppt-battery-management-system) |
| Multiplexed 8-Zone Soil Irrigation Scheduler | Mega | CD74HC4067-multiplexed soil zones with rain-adaptive scheduling and closed-loop valve shutoff. | [advanced-complex/06-multiplexed-soil-irrigation-scheduler](advanced-complex/06-multiplexed-soil-irrigation-scheduler) |
| IMU Gesture Recognition Controller | Nano | On-device nearest-neighbor gesture classifier drives a pan-tilt mount and a light relay. | [advanced-complex/07-imu-gesture-recognition-controller](advanced-complex/07-imu-gesture-recognition-controller) |
| Audio Spectrum LED Matrix Visualizer | Uno | Real-time FFT audio spectrum bar graph on a MAX7219 matrix with loud-event SD logging. | [advanced-complex/08-audio-spectrum-led-matrix-visualizer](advanced-complex/08-audio-spectrum-led-matrix-visualizer) |
| Autonomous Maze-Solving Robot | Mega | Flood-fill maze solver fusing IR, ultrasonic, encoders, and IMU, with an EEPROM-stored speed run. | [advanced-complex/09-autonomous-maze-solving-robot](advanced-complex/09-autonomous-maze-solving-robot) |
| Multi-Sensor Weather Buoy Datalogger | Uno Q | Fuses 6 weather/water sensors into a timestamped SD log with a live Ethernet dashboard. | [advanced-complex/10-multi-sensor-weather-buoy-datalogger](advanced-complex/10-multi-sensor-weather-buoy-datalogger) |
| RS-485 Sensor Mesh with Uno Q Hub | 3x Uno + Uno Q | Three field nodes report over RS-485 to a hub that logs to SD and serves a dashboard. | [advanced-complex/11-rs485-sensor-mesh-uno-q-hub](advanced-complex/11-rs485-sensor-mesh-uno-q-hub) |
| Dual-Board CNC: Motion/UI Split | Mega + Uno | A motion controller and a UI/G-code-feeder board coordinate over a flow-controlled UART protocol. | [advanced-complex/12-dual-mega-cnc-motion-ui-split](advanced-complex/12-dual-mega-cnc-motion-ui-split) |
| I2C Multi-Drop Industrial Node Network | Uno Q + 3x Nano | A hub polls 3 addressed I2C slave nodes (relay bank, load cell, thermocouple) and serves a dashboard. | [advanced-complex/13-i2c-multidrop-industrial-node-network](advanced-complex/13-i2c-multidrop-industrial-node-network) |
| Conveyor Sorting Dual-Board Automation | Uno + Mega | A color-sorting Uno and a PID belt-speed Mega coordinate over I2C for automated sorting. | [advanced-complex/14-conveyor-sorting-dual-board-automation](advanced-complex/14-conveyor-sorting-dual-board-automation) |
| RS-485 Distributed Greenhouse Zone Network | 3x Uno + Mega | Independent local PID climate zones report to a supervisory RS-485 hub with SD logging + dashboard. | [advanced-complex/15-rs485-distributed-greenhouse-zone-network](advanced-complex/15-rs485-distributed-greenhouse-zone-network) |
| nRF24 Swarm Formation Robots | Uno + 2x Nano | A leader broadcasts pose over nRF24L01 while followers hold formation via closed-loop ultrasonic control. | [advanced-complex/16-nrf24-swarm-formation-robots](advanced-complex/16-nrf24-swarm-formation-robots) |
| Dual-Uno Elevator PLC Simulation | 2x Uno | A car controller and a call panel coordinate over I2C to simulate a small elevator PLC. | [advanced-complex/17-dual-uno-elevator-plc-simulation](advanced-complex/17-dual-uno-elevator-plc-simulation) |
| RS-485 Energy Monitoring Mesh | 3x Nano + Mega | Non-invasive CT-clamp meter nodes report real power over RS-485 to a logging/dashboard hub. | [advanced-complex/18-rs485-energy-monitoring-mesh](advanced-complex/18-rs485-energy-monitoring-mesh) |
| Master/Slave Serial Command Queue: Warehouse AGV | Uno + Mega | A dispatch console queues jobs to a line-following AGV over a checksummed, flow-controlled UART protocol. | [advanced-complex/19-master-slave-serial-command-queue-warehouse-agv](advanced-complex/19-master-slave-serial-command-queue-warehouse-agv) |
| Redundant Dual-MCU Failover Monitoring System | 2x Uno | Primary/standby boards share sensors and actuators with heartbeat-based automatic failover. | [advanced-complex/20-redundant-failover-monitoring-system](advanced-complex/20-redundant-failover-monitoring-system) |
| Security Fusion with ESP32 WiFi Relay | Mega + ESP32 DevKit | Sensor-fusion security alerts relayed to MQTT over WiFi by a companion ESP32 (cross-repo flagship). | [advanced-complex/21-security-fusion-esp32-relay](advanced-complex/21-security-fusion-esp32-relay) |
| Smart Agriculture Command Platform | Mega | Flagship irrigation + climate platform with an SD "database," token-auth REST API, and a watchdog. | [advanced-complex/22-smart-agriculture-command-platform](advanced-complex/22-smart-agriculture-command-platform) |
| Home Energy Resilience Management System | Mega | Automatic grid/solar/battery source selection and load shedding with EEPROM-persisted state. | [advanced-complex/23-home-energy-resilience-management-system](advanced-complex/23-home-energy-resilience-management-system) |
| Cold Chain Logistics Monitor | Mega | Store-and-forward shipment monitoring pushed over a SIM800L cellular link, with local cooling response. | [advanced-complex/24-cold-chain-logistics-monitor](advanced-complex/24-cold-chain-logistics-monitor) |
| Autonomous Water Quality Monitoring Buoy | Uno Q | Fuses 5 water-quality sensors with closed-loop aerator response and a token-auth dashboard. | [advanced-complex/25-autonomous-water-quality-monitoring-buoy](advanced-complex/25-autonomous-water-quality-monitoring-buoy) |

## Related Cross-Repo Projects

An Arduino node also appears as a sensor client in [Distributed Sensor Hub with REST API](../RASPBERRY-PI-PROJECTS/advanced-complex/21-distributed-sensor-hub-rest-api/README.md) (lead project in RASPBERRY-PI-PROJECTS).

## Board Usage Summary

| Board | Project Count |
|---|---|
| Uno | 8 |
| Nano | 8 |
| Uno Q | 5 |
| Mega | 5 |

(The dual-board wireless security system uses one Nano and one Uno together, and is counted toward both totals above.)

## License

This repository is licensed under the [PolyForm Strict License 1.0.0](LICENSE) — restricted, non-redistributable.

## Author

Hashibuzzaman Bhuiyan

