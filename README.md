# Shockee - Motorbike Suspension Dyno Software

A Qt-based application for real-time data collection and analysis of motorbike suspension performance using Arduino sensors.

## Features

- **Real-time Data Collection**: Captures data from linear potentiometer, load cell (HX711), and rotary encoder
- **Live Plotting**: Real-time visualization of position, force, and encoder data
- **Force vs Position Analysis**: Generate compression/rebound curves for suspension analysis
- **Data Logging**: Save test sessions with metadata for later analysis
- **Session Comparison**: A/B comparison with overlay plots
- **Export Capabilities**: Export data to CSV, Excel, and PDF formats
- **Calibration Tools**: Built-in calibration for all sensors
- **Velocity Calculation**: Real-time velocity calculation from position data

## Hardware Requirements

### Arduino Setup
- Arduino Uno R3
- Linear potentiometer (75mm travel)
- Load cell (400kg capacity) with HX711 amplifier
- Optical rotary encoder
- USB cable for serial communication

### Wiring Diagram
```
Arduino Uno R3:
- A0: Linear potentiometer wiper
- Pin 2: Encoder A (interrupt)
- Pin 3: Encoder B (interrupt)
- Pin 4: HX711 DOUT
- Pin 5: HX711 SCK
- 5V/GND: Power rails for sensors
```

## Software Requirements

- Qt6 (Core, Widgets, SerialPort, PrintSupport)
- CMake 3.16+
- C++17 compiler
- Arduino IDE (for uploading sketch)

## Installation

### Ubuntu 24.04 LTS (Recommended)

**Quick Install:**
```bash
# Build and install .deb package
./build-ubuntu.sh

# Install the package
sudo dpkg -i ../shockee_1.0.0-1_amd64.deb
sudo apt-get install -f  # Fix any dependencies

# Run the application
shockee
```

**Manual Dependencies:**
```bash
sudo apt update
sudo apt install qt6-base-dev qt6-serialport-dev libqt6printsupport6-dev
```

### macOS

**Using Homebrew:**
```bash
brew install qt6 cmake
```

**Build macOS App Bundle:**
```bash
./deploy-macos.sh
# Creates Shockee.app and .dmg installer
```

### Arduino Sketch
1. Install the HX711 library in Arduino IDE
2. Upload `arduino/shockee_sensors.ino` to your Arduino Uno R3
3. Connect sensors according to the wiring diagram

### Qt Application (From Source)
```bash
git clone https://github.com/yourusername/shockee-qt.git
cd shockee-qt
mkdir build && cd build
cmake ..
make
```

## Usage

### Getting Started
1. Connect your Arduino with sensors via USB
2. Launch Shockee application
3. Select the correct serial port and connect
4. Calibrate sensors using the Calibration dialog
5. Start recording data for your suspension tests

### Calibration
- **Load Cell**: Use known weights to calibrate force readings
- **Potentiometer**: Set full stroke positions (0mm and 75mm)
- **Encoder**: Reset to zero at desired reference position

### Test Sessions
1. Click "Start Recording" to begin data capture
2. Perform your suspension test (max 2 minutes)
3. Click "Stop Recording" when complete
4. Save session with descriptive name and metadata
5. Export data for further analysis

### Data Analysis
- **Real-time Plots**: Monitor all sensors during testing
- **Force vs Position**: Analyze compression/rebound characteristics
- **Velocity Analysis**: Study damping performance
- **Session Comparison**: Compare different setups or settings

## Data Format

The application captures data at 100Hz with the following parameters:
- **Timestamp**: Milliseconds since test start
- **Position**: Linear position in mm (0-75mm range)
- **Force**: Applied force in kg (0-400kg range)
- **Encoder**: Rotary encoder pulse count
- **Velocity**: Calculated velocity in mm/s

## File Formats

### Session Files (.json)
Native format containing all data and metadata
```json
{
  "name": "Test Session",
  "timestamp": "2024-01-01T12:00:00.000Z",
  "metadata": {
    "strut_info": "Ohlins TTX36",
    "spring_rate": 95.0,
    "damping_setting": 12
  },
  "data": [...]
}
```

### CSV Export
Comma-separated values for external analysis:
```
timestamp,position_mm,force_kg,encoder_pulses,velocity_mm_s
0,0.00,0.00,0,0.00
10,1.23,15.67,45,123.0
...
```

## Contributing

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Add tests if applicable
5. Submit a pull request

## License

This project is licensed under the MIT License - see the LICENSE file for details.

## Troubleshooting

### Common Issues
- **Serial Port Not Found**: Check Arduino connection and drivers
- **No Data Received**: Verify Arduino sketch upload and sensor wiring
- **Calibration Issues**: Ensure sensors are properly connected and powered
- **Build Errors**: Check Qt6 installation and CMake configuration

### Support
For issues and feature requests, please use the GitHub issue tracker.