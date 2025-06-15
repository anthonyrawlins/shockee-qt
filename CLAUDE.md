# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build Commands

```bash
# Build the application
cd build
cmake ..
make

# Clean build
rm -rf build && mkdir build && cd build
cmake .. && make
```

## Project Architecture

This is a Qt6-based C++ application for real-time motorbike suspension testing using Arduino sensors.

### Core Components

- **MainWindow**: Central UI controller with tabbed interface (real-time, analysis, comparison)
- **SerialCommunicator**: Handles Arduino serial communication and sensor data parsing
- **DataLogger**: Session management, file I/O, and data analysis functions
- **PlotWidget**: Real-time plotting of sensor data
- **CalibrationDialog**: Sensor calibration interface

### Data Flow

1. Arduino collects sensor data at 100Hz (potentiometer, load cell, rotary encoder)
2. SerialCommunicator parses CSV data and calculates velocity
3. MainWindow updates real-time plots and displays
4. DataLogger saves sessions to JSON files with metadata
5. Export functionality supports CSV and Excel formats

### Key Data Structures

- **SensorData**: timestamp, position (mm), force (kg), encoder pulses, velocity (mm/s)
- **Session**: contains data vector plus metadata (strut info, spring rate, damping settings)

### Hardware Interface

The application communicates with Arduino Uno R3 via serial port at 9600 baud. Arduino sketch located at `arduino/shockee_sensors.ino` must be uploaded first.

### File Structure

- `src/`: C++ source files
- `ui/`: Qt UI files
- `arduino/`: Arduino sketch for sensor data collection
- `sessions/`: JSON session files (created at runtime)
- `build/`: CMake build directory