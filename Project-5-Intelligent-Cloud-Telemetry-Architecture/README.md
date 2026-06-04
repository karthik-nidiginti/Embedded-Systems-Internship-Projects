# Intelligent Cloud Telemetry Architecture

## Overview

This project implements a smart IoT telemetry system using ESP32 and Blynk Cloud.

Unlike conventional telemetry systems that send data continuously, this solution transmits data only when:

- State changes occur
- Voltage threshold violations happen
- Significant sensor changes are detected
- Network events occur

The system supports WiFi reconnection, cloud reconnection, signal strength monitoring, and fault-tolerant operation.

---

## Features

- Event-driven telemetry
- Voltage monitoring
- WiFi RSSI monitoring
- Cloud connection status indication
- Automatic WiFi reconnect
- Automatic Blynk reconnect
- Serial diagnostics
- Green/Red status LEDs

---

## Hardware Used

- ESP32 DevKit V1
- Potentiometer (Sensor Simulator)
- Green LED
- Red LED
- 2 × 220Ω Resistors

---

## Pin Configuration

| Component | ESP32 Pin |
|-----------|-----------|
| Sensor | GPIO32 |
| Green LED | GPIO25 |
| Red LED | GPIO26 |

---

## Wokwi Simulation

Add your Wokwi project link here:

https://wokwi.com/projects/YOUR_PROJECT_ID

---

## Blynk Datastreams

| Virtual Pin | Purpose |
|-------------|---------|
| V0 | Voltage |
| V1 | State |
| V2 | WiFi RSSI |
| V3 | Event Counter |
| V5 | Queue Size |
| V6 | Cloud Status |
| V7 | Uptime |

---

## Author

Karthik Nidiginti
