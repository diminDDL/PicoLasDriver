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

**Image** *(to be replaced with an actual image)*

- **"+" and "-" buttons**: Increase or decrease values. They accelerate when held.
- **"Single/Pulsed" modes**: Toggle single pulse operation.
- **"Global/Local pulse count"**: View total pulse count and current session count.
- **"ADC readout"**: Display the raw 12-bit ADC value from the photodiode amplifier.
- **"Lock icon"**: Lock or unlock the device to prevent accidental value changes.
- **"IO1-4"**: IO pin toggle buttons.
- **"Enable/Disable"**: Control the pulse output.

⚠️ **Important**: Always turn off the output when adjusting settings to prevent potential microcontroller glitches.

## Hardware & Communication

- The board connects to the Raspberry Pi via USB, with a communication frequency of every 100ms.
- Auto-reconnect and fault indications in the Python GUI.
- LED indicators for the board's internal states.
- GPIO pins provide a high impedance 5V when active.
- Over-voltage protection up to 5V.
- Photodiode amplifier module design and guidance provided.

**Image** *(to be replaced with an actual image showcasing the pinout)*

## Operating Mechanism

The board keeps the Picolas driver's enable pin high and uses the trigger pin to generate impulses. An analog DAC sets the voltage to control the output current limit. If the E-STOP pin is triggered, the system becomes non-functional until a power cycle.

## License

This project is licensed under the MIT License:

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

---

**Note**: Remember to replace the placeholder **Image** with the actual images showcasing the GUI layout and board pinout for clarity.
