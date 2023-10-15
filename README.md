# CAN Joystick Project

This project is designed for controlling a multi-color LED system using a joystick interface and the CAN (Controller Area Network) protocol. The joystick allows you to perform various tasks, including manual color selection, automatic color transitions, and more. The system uses an ESP32 and the MCP2515 CAN controller.

## Table of Contents
- [Overview](#overview)
- [Hardware](#hardware)
- [Software](#software)
- [Usage](#usage)

## Overview

The CAN Joystick Project is a ESP32-based system that provides control over a multi-color LED system using a joystick and the CAN protocol. It offers the following features:

- Manual mode: Control LED colors and effects by rotating the joystick.
- Color change mode: Change colors by rotating the joystick.
- Automatic mode: Automatically cycle through preset colors.

## Hardware

- ESP32 board
- MCP2515 CAN controller
- Joystick interface
- Multi-color LEDs

## Software

The software for this project is written in Arduino code and uses the SimpleJ1939 library for handling the CAN communication. Here are the main components of the software:

## Usage

1. Connect the hardware components (ESP32, MCP2515, joystick, and LEDs) as per the project's requirements.
2. Upload the provided Arduino code to the board.
3. Use the joystick to control the LEDs as follows:
   - In manual mode, rotate the joystick to control the LED colors and effects.
   - In color change mode, rotate the joystick to change colors.
   - In automatic mode, the system will automatically cycle through preset colors.
