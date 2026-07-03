# Mhi2MQTT

Control your Mitsubishi Heavy Industries Air Conditioner locally with Home Assistant using ESP32-S3. Communicates directly with the A/C unit via SPI on the CNS port — no cloud, no Mitsubishi app required.

## Features

- Bi-directional control — commands and true A/C state are kept in sync
- Home Assistant MQTT Discovery (auto-configures climate entity)
- Captive portal on first boot for WiFi + MQTT setup
- OTA firmware updates
- Triple-reset factory reset

## Supported Controls

| Control | Values |
|---|---|
| Power | ON / OFF |
| Mode | AUTO / COOL / HEAT / DRY / FAN |
| Temperature setpoint | °C or °F |
| Fan speed | AUTO / 1 / 2 / 3 / 4 |
| Vertical vane | SWING / 1–4 |
| Horizontal vane | SWING / 1–6 / WIDE / SPOT |

## MQTT Status

All fields are published to `<topic>/<friendly_name>/status` as JSON.

| Field | Unit | Description |
|---|---|---|
| `roomTemperature` | °C/°F | Indoor air temperature |
| `outsideTemperature` | °C/°F | Outdoor air temperature |
| `internalCoilTemperature` | °C/°F | Indoor U-bend coil temperature |
| `fanRPM` | raw | Indoor fan speed (raw sensor value) |
| `compressorFrequency` | Hz | Compressor operating frequency |
| `currentAmps` | A | AC current draw |
| `energyUsed` | kWh | Cumulative energy (AC internal counter, 0.25 kWh resolution) |
| `defrosting` | bool | Defrost cycle active |
| `compressorProtection` | 0–255 | Protection/fault state (0 = normal) |
| `indoorRunHours` | h | Indoor unit lifetime run hours |
| `compressorRunHours` | h | Compressor lifetime run hours |

## Hardware

- **MCU:** ESP32-S3
- **Connection:** SPI slave on the A/C CNS port
- **Pins (default):** MOSI=43, MISO=44, SCLK=38, CS_IN=10, CS_OUT=9

## Software

Built with PlatformIO (Arduino + ESP-IDF dual framework, pioarduino platform with IDF 5.x).

```bash
# Build
pio run

# Flash (hold BOOT, press RESET first for first-time install)
pio run -t erase && pio run -t upload

# Monitor serial output
pio device monitor
```

## First Boot

On first boot (or after factory reset) the device starts a captive portal AP named `MHI_<MAC>`. Connect to it, enter your WiFi credentials and MQTT broker details, then save. The device reboots and connects automatically.

## Links

- [Daikin2MQTT](https://github.com/maxmacstn/daikin2MQTT) — original project this was forked from
- [Mitsubishi2MQTT](https://github.com/maxmacstn/mitsubishi2MQTT) — inspiration
- [MHI-AC-Ctrl (ESP32)](https://github.com/hberntsen/mhi-ac-ctrl-esp32) — SPI driver used in this project
- [MHI-AC-Ctrl (ESP8266)](https://github.com/absalom-muc/MHI-AC-Ctrl) — original MHI protocol research
