# Intelligent Embedded HMI & Diagnostic Interface

## Overview

The Intelligent Embedded HMI & Diagnostic Interface is an ESP32-based embedded monitoring system designed to provide a professional Human-Machine Interface (HMI) for battery diagnostics and protection monitoring.

The system uses a 16x2 I2C LCD display to automatically rotate between multiple diagnostic screens, presenting real-time battery information, analytics, protection status, and fault diagnostics. During critical fault conditions, the interface automatically overrides normal screen rotation and displays high-priority warning messages to ensure operator awareness.

## Objectives

* Develop a professional embedded HMI using an LCD display.
* Display real-time battery monitoring information.
* Implement automatic multi-screen rotation.
* Provide fault-priority screen override functionality.
* Ensure smooth and flicker-free LCD updates.
* Support battery diagnostics and protection monitoring.

## Features

* Real-time monitoring of four battery cells
* Automatic LCD screen rotation
* Battery pack average voltage calculation
* Battery imbalance percentage calculation
* Strongest and weakest cell identification
* Health status monitoring
* Fault detection and diagnostics
* Fault-priority warning screens
* Flicker-free LCD refresh handling
* Blynk IoT cloud monitoring
* ESP32-based implementation

## Hardware Components

* ESP32 Development Board
* 16x2 I2C LCD Display
* 4 Potentiometers (Battery Cell Simulation)
* Green LED
* Yellow LED
* Red LED
* Buzzer
* Relay Module
* Jumper Wires

## Software Tools

* Arduino IDE
* Wokwi Simulator
* Blynk IoT Platform

## Pin Configuration

| Component    | ESP32 Pin |
| ------------ | --------- |
| Cell 1 Input | GPIO 32   |
| Cell 2 Input | GPIO 33   |
| Cell 3 Input | GPIO 34   |
| Cell 4 Input | GPIO 35   |
| Green LED    | GPIO 12   |
| Yellow LED   | GPIO 13   |
| Red LED      | GPIO 14   |
| Buzzer       | GPIO 15   |
| Relay        | GPIO 16   |
| LCD SDA      | GPIO 21   |
| LCD SCL      | GPIO 22   |

## LCD Diagnostic Screens

### Screen 1

* Individual Cell Voltages

### Screen 2

* Pack Average Voltage
* Imbalance Percentage

### Screen 3

* Strongest Cell
* Weakest Cell

### Screen 4

* System Health Status

### Fault Screen

* Critical Warning Messages
* Protection Status
* Diagnostic Information

## Blynk Datastreams

| Virtual Pin | Description          |
| ----------- | -------------------- |
| V0          | Cell 1 Voltage       |
| V1          | Cell 2 Voltage       |
| V2          | Cell 3 Voltage       |
| V3          | Cell 4 Voltage       |
| V4          | Pack Average Voltage |
| V5          | Imbalance Percentage |
| V6          | Health Status        |
| V7          | Strongest Cell       |
| V8          | Weakest Cell         |

## Applications

* Battery Management Systems (BMS)
* Electric Vehicle Monitoring
* Energy Storage Systems
* Embedded Diagnostic Interfaces
* Industrial Monitoring Systems
* IoT-Based Battery Analytics

Wokwi Simulation
Add your Wokwi project link here:

https://wokwi.com/projects/465378963580763137

## Author

Karthik Nidiginti

## Internship Project

Embedded Systems Internship Project – Intelligent Embedded HMI & Diagnostic Interface
