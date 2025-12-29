# ESP32-2432S028C BT Speaker

This project is a Bluetooth (BT) speaker based on the [yoradio project](https://github.com/e2002/yoradio). It uses a dual partition approach for firmware management. The BT firmware is stored in the OTA partition.

## Features

- Bluetooth audio streaming
- Dual partition firmware (BT and WiFi modes)
- Touch panel control for switching modes
- Based on ESP32 microcontroller

## Hardware Requirements

- ESP32-2432S028C board
- Touch panel (TP) for mode switching
- Speakers and audio components

## Software Requirements

- PlatformIO IDE
- ESP32 development tools

## Installation

1. Clone this repository.
2. Open the project in PlatformIO.
3. Install required libraries (Adafruit libraries, ESP32-A2DP, etc.).

## Building and Uploading

### Upload BT Firmware

To upload the BT firmware:

1. Open PlatformIO.
2. Press `Shift + Ctrl + P` to open the command palette.
3. Select "Tasks: Run Task".
4. Choose "Upload BT firmware" from the list.
5. The firmware will be uploaded to the OTA partition.

## Usage

- Power on the device.
- Long press the touch panel (TP) to switch between BT and WiFi modes.
- In BT mode, connect your device via Bluetooth for audio streaming.
- In WiFi mode, use the web interface for control.

## Project Structure

- `src/`: Main source code
- `lib/`: Libraries (Adafruit, ESP32-A2DP, etc.)
- `data/`: Data files
- `include/`: Header files
- `test/`: Test files

## License

This project is based on yoradio. Check the original repository for licensing information.