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

# First-time flash (replacing ESPHome or clean install) — hold BOOT, press RESET first
pio run -t erase && pio run -t upload

# Open serial monitor
pio device monitor

# Build, flash, and monitor in one step
pio run -t upload && pio device monitor

# Clean build artifacts (required after sdkconfig.defaults changes)
pio run -t clean

# Run native (host) unit tests — no hardware needed
pio test -e native

# Flash over the network (web OTA; espota/UDP-3232 is firewall-blocked cross-VLAN)
pio run && curl -F "file=@.pio/build/wifikit-serial-esp32-s3/firmware.bin" http://10.1.50.7/upload
```

Target environment: `wifikit-serial-esp32-s3` (ESP32-S3, Arduino + ESP-IDF dual framework, pioarduino platform with IDF 5.x).

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
- `CONFIG_AUTOSTART_ARDUINO=1` must stay in `build_flags` — it gates the Arduino `app_main` that calls `setup()`/`loop()`; removing it silently breaks boot.
- `sdkconfig.defaults` sets `CONFIG_FREERTOS_HZ=1000` and 8 MB flash config; any changes require `pio run -t clean` to regenerate the sdkconfig.
- Native unit tests (`pio test -e native`, Unity) cover the pure logic: string↔enum mappings (`src/mhi_mappings.cpp`) and frame checksum/temperature codecs (`src/MHI-AC-Ctrl/mhi-frame.cpp`). Tests `#include` the extracted `.cpp` directly — the `[env:native]` build excludes `src/` because `main.cpp` and the driver need Arduino/ESP-IDF headers. Hardware behavior is still validated by flashing and monitoring serial output.

## MHI Driver Safety Rules

- `mhi_ac::init()` spawns a permanent FreeRTOS task — call it exactly **once** per boot; calling twice is undefined behavior.
- `operation_data_state.<field>.enabled = true` must be set **before** `mhi_ac::init()`; setting after has no effect.
- `active_mode_set(true)` must be called **after** `init()`; without it the SPI task sends 0xFF filler.
- Always access `spi_state` via the snapshot semaphore: `snapshot_semaphore_take()` → read → `set_snapshot_as_previous()` → `snapshot_semaphore_give()`.
- `operation_data_state` has its own semaphore: `value_semaphore_take()` / `value_semaphore_give()`.
- MQTT string values in `HVACSettings` (power/mode/fan/vane) must be string literals or `strdup()`'d — never `.c_str()` of a local `String`.

## MHI MQTT State

- `pollMhiState()` in `main.cpp` populates `currentSettings` + `currentStatus` each loop from the SPI snapshot.
- Published MQTT fields: `roomTemperature`, `outsideTemperature`, `internalCoilTemperature`, `fanRPM`, `compressorFrequency`, `currentAmps`, `energyUsed`, `defrosting`, `compressorProtection`, `indoorRunHours`, `compressorRunHours`.
- `energyUsed` reads the AC's own internal energy accumulator (operation data DB9=0x94, `raw/4` = kWh, 0.25 kWh resolution). Do not replace with the `Energy` class — its `voltage` field is uninitialised in this project.
