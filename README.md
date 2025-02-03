# Mhi2MQTT

Control your Mitsubishi Heavy Industries Air Conditioner locally with Home Assistant using ESP32. Communicates directly with A/C using SPI Communication via CNS port.

## Features

- Bi-Directional control, sync true A/C status to Home Assistant.
- Support Home Assistant MQTT Discovery.

## Functions

- Basic Operation: Power, Mode, Temp setpoint, Fan speed, Vertical swing.
- Status Monitor: Room Temp, Outside Temp, FCU Coil Temp, FCU Fan RPM, Compressor Frequency.

## Software

The code in this repository is based on Arduino framework, and implemented on PlatformIO IDE.

## Links

Many thanks for all the work from the links below for a detailed guide and inspiration for building this project.

- [Daikin2MQTT](https://github.com/maxmacstn/daikin2MQTT)
- [Mitsubishi2MQTT](https://github.com/maxmacstn/mitsubishi2MQTT)
- [mhi-ac-ctrl](https://github.com/absalom-muc/MHI-AC-Ctrl)
- [mhi-ac-ctrl-esp32-c3](https://github.com/hberntsen/mhi-ac-ctrl-esp32-c3)
