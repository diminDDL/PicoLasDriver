# Picolas Laser Diode Controller/Driver

This repository houses the necessary components for a touchscreen-based controller/driver for the Picolas series of laser diode boards, including hardware schematics, embedded firmware, and a Python GUI.

## Features

- Supports control of analog models with trigger pins and analog voltage for adjusting the current setpoint.
- Designed to work seamlessly with a Raspberry Pi SBC and a touchscreen.
- Configuration flexibility through JSON file.

## Getting Started

### Prerequisites

- Raspberry Pi with a touchscreen
- Picolas series laser diode board (analog models with trigger pins)

### Installation

1. Clone the repository.
2. Follow the setups in [howto.md](https://github.com/diminDDL/PicoLasDriver/blob/main/howto.md)
3. Configure a startup script to initiate the Python program automatically on boot. Additionally, it's advisable to disable the touchscreen until the program starts.
4. Consider enabling the overlay filesystem to safeguard your data.

## GUI Layout & Functions

![GUI](https://github.com/diminDDL/PicoLasDriver/blob/main/images/GUI.png?raw=true)

- **"+" and "-" buttons**: Increase or decrease values. They accelerate when held.
- **"Single/Pulsed" modes**: Toggle single pulse operation.
- **"Global/Local pulse count"**: View total pulse count and current session count.
- **"ADC readout"**: Display the raw 12-bit ADC value from the photodiode amplifier.
- **"Lock icon"**: Lock or unlock the device to prevent accidental value changes.
- **"IO1-4"**: IO pin toggle buttons.
- **"Enable/Disable"**: Control the pulse output.
- **Error readout**: Reads out the errors and other state information.

⚠️ **Important**: Always turn off the output when adjusting settings to prevent potential microcontroller glitches.

## Hardware & Communication

![Schematic](https://github.com/diminDDL/PicoLasDriver/blob/main/images/schematic.png?raw=true)

- The board connects to the Raspberry Pi via USB, with a communication frequency of every 100ms.
- Auto-reconnect and fault indications in the Python GUI.
- LED indicators for the board's internal states.
- GPIO pins provide a high impedance 5V when active.
- Over-voltage protection up to 5V.
- Photodiode amplifier module design and guidance provided.

### Pinout of the connector:
![Schematic](https://github.com/diminDDL/PicoLasDriver/blob/main/images/pinout.png?raw=true)


## Operating Mechanism

The board keeps the Picolas driver's enable pin high and uses the trigger pin to generate impulses. An analog DAC sets the voltage to control the output current limit. If the E-STOP pin is triggered, the system becomes non-functional until a power cycle.

## License

This project is licensed under the MIT License.
