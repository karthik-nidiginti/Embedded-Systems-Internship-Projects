# Event-Driven Safety Protection Kernel

## Overview

This project implements an automotive-grade battery safety protection controller using ESP32 and a fully non-blocking event-driven architecture.

The system detects:

- Weak cell voltage
- Overvoltage
- Sensor failures
- Rapid voltage fluctuations

It performs:

- Relay cutoff protection
- Buzzer warning alerts
- LCD warning display
- Stable recovery logic
- Anti-relay chatter protection

The firmware is built completely using millis()-based timing without delay().

---

## Hardware Used

- ESP32
- 16x2 I2C LCD
- Relay Module
- Buzzer
- LEDs
- Potentiometers
- Blynk IoT

---

## Features

- Real-time 4-cell monitoring
- Event-driven firmware
- Non-blocking execution
- Recovery state management
- Relay safety protection
- LCD status display
- Blynk cloud monitoring
- Automotive-style fault handling

---

## Blynk Datastreams

| Datastream | Purpose |
|---|---|
| V0 | Cell1 Voltage |
| V1 | Cell2 Voltage |
| V2 | Cell3 Voltage |
| V3 | Cell4 Voltage |
| V4 | System State |
| V5 | Relay Status |
| V6 | Warning Counter |

---

## Author

Karthik Nidiginti
