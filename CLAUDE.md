# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## OpenWolf

@.wolf/OPENWOLF.md

This project uses OpenWolf for context management. Read and follow `.wolf/OPENWOLF.md` every session. Check `.wolf/cerebrum.md` before generating code. Check `.wolf/anatomy.md` before reading files.

## Project Overview

ESP32-S3 firmware that bridges a Mitsubishi Heavy Industries (MHI) air conditioner to MQTT/Home Assistant. The A/C is controlled via SPI on its CNS port. Configuration is stored in SPIFFS. The web UI provides a captive portal on first boot and a settings dashboard thereafter.

## Build Commands

```bash
# Build firmware
pio run

# Build and flash to device (macOS USB)
pio run -t upload

# Open serial monitor
pio device monitor

# Build, flash, and monitor in one step
pio run -t upload && pio device monitor

# Clean build artifacts
pio run -t clean
```

Target environment: `wifikit-serial-esp32-s3` (ESP32-S3, Arduino + ESP-IDF dual framework, `espressif32 @ 6.5.0`).

Upload port defaults to `/dev/cu.usbmodem*`. For OTA, set `upload_port = <device IP>` in `platformio.ini`.

## Architecture

### Data Flow

```
MHI A/C unit (SPI/CNS port)
        ↕  SPI (ESP-IDF SPI2_HOST)
src/MHI-AC-Ctrl/            ← low-level SPI frame encode/decode + FreeRTOS task
        ↕  HVACSettings / HVACStatus callbacks
src/main.cpp                ← orchestration layer
        ↕ MQTT (PubSubClient)
Home Assistant / MQTT broker
```

### Key Modules

- **`src/main.cpp`** — Main application: WiFi (captive portal on first boot), web server (port 80), MQTT client, OTA, Home Assistant MQTT Discovery, watchdog, multi-reset factory-reset detection.
- **`src/MHI-AC-Ctrl/`** — MHI A/C SPI driver. Uses raw ESP-IDF SPI (not Arduino SPI) and FreeRTOS tasks. `MHI-AC-Ctrl-core.cpp/.h` is the entry point. `MHI-AC-CTRL-operation-data.h` maps SPI frame bytes to AC state.
- **`src/DaikinController/`** — Daikin serial controller (currently unused; the `#include` in `main.cpp` is commented out). Do not delete — it may be re-enabled.
- **`src/config.h`** — All global constants: SPIFFS file paths (`/wifi.json`, `/mqtt.json`, `/unit.json`, `/others.json`), MQTT topic defaults, pin definitions, version strings.
- **`src/html_*.h` / `src/javascript_common.h`** — Web UI templates as C string literals.
- **`src/languages/*.h`** — Localization strings (7 languages). Selected at compile time via `config.h`.
- **`src/logger.h/.cpp`** — Serial logging wrapper.

### Configuration Storage (SPIFFS)

All persistent config lives in SPIFFS JSON files:
- `/wifi.json` — SSID, password
- `/mqtt.json` — broker, port, credentials, friendly name
- `/unit.json` — temperature unit, language, A/C-specific settings
- `/others.json` — miscellaneous settings

### First-Boot Flow

On first boot (no `wifi.json`), the device launches a captive portal AP (`MHI_<MAC>`). The user configures WiFi + MQTT via the web UI, which saves to SPIFFS and reboots into normal operation.

### Factory Reset

Triple-reset within the reset window (handled by `ESP_MultiResetDetector`) triggers `wifiFactoryReset()` which wipes SPIFFS config files.

## Framework Notes

- Uses **both** Arduino and ESP-IDF frameworks simultaneously (`framework = arduino, espidf`). MHI-AC-Ctrl uses raw ESP-IDF SPI and FreeRTOS APIs directly.
- Loop task stack is explicitly set to 16 KB (`SET_LOOP_TASK_STACK_SIZE(16 * 1024)`) — do not reduce; 8 KB causes crashes.
- There are no unit tests. Validation is done by flashing to hardware and monitoring serial output.
