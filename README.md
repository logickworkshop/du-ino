# DU-INO Arduino Library

## Overview

This is the official Arduino library for the DU MDLR DU-INO Eurorack module.

## Getting Started

Download the [Arduino IDE](https://www.arduino.cc/en/Main/Software) for your system, clone this repository into the `libraries` subdirectory, and start hacking!

A growing collection of Detroit Underground functions are being added as examples, which can be accessed from **File** - **Examples** - **DU-INO** in the IDE once the library is installed.

Note that the DU-INO module has a small switch on the lower left (from the front) of the PCB that must be switched to the *up* position for programming and the *down* position to run.

## Modules

The following modules are currently available in this library:

- Core function module, providing a base for any kind of function.
- MCP4922 DAC driver (used by core function module).
- Click encoder driver (used by core function module).
- SH1106 OLED display driver, with graphics functions and font library.
- GUI widget module, with display and interaction primitives.
- EEPROM parameter saving and loading widget module.
- Clock module, for functions requiring clock sync.
- DSP module, for filters and other signal processing.
- Musical scale module, for functions dealing with musical notes.

## Supported Arduino Boards

The following boards are currently supported:

- Arduino Uno R3

Compatible boards and other Arduino boards may work; however, some hardware-specific registers and settings may need to be modified for different microcontroller models, and some functions may exceed program memory, RAM, or EEPROM limitations on other devices.

We are hoping to expand this list. Pull requests with compile-time checks adding support for different devices are very welcome.
