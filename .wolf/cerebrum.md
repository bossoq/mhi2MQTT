# Cerebrum

> OpenWolf's learning memory. Updated automatically as the AI learns from interactions.
> Do not edit manually unless correcting an error.
> Last updated: 2026-06-30

## User Preferences

<!-- How the user likes things done. Code style, tools, patterns, communication. -->

## Key Learnings

- **Project:** mhi2MQTT
- **Description:** Control your Mitsubishi Heavy Industries Air Conditioner locally with Home Assistant using ESP32. Communicates directly with A/C using SPI Communication via CNS port.

### MQTT / WiFi reliability (2026-07-06)

- **keepalive = 30s** is the correct value. The previous 120s gave a 180s LWT delay and 135s silent-drop detection window — both too slow for HA to reflect reality.
- **WiFi.onEvent callback is not PubSubClient-safe** — it runs in lwIP task context. Never call `mqtt_client` methods directly from it. Use a `volatile bool wifi_disconnected` flag and act on it in `loop()`.
- **Availability must be re-published periodically** (every 60s in `loop()`, inside MQTT-connected branch). Single publish on `mqttConnect()` is lost if broker restarts and clears retained messages. The payload must mirror `_debugMode`: `!_debugMode ? mqtt_payload_available : mqtt_payload_unavailable` — same as the connect-time publish.
- **`hpStatusChanged()` is called from `loop()`**, not directly from the MHI FreeRTOS task, so `mqtt_client.publish()` inside it is thread-safe. The FreeRTOS task only writes to the SPI snapshot under its own semaphore.

## Do-Not-Repeat

<!-- Mistakes made and corrected. Each entry prevents the same mistake recurring. -->
<!-- Format: [YYYY-MM-DD] Description of what went wrong and what to do instead. -->

- [2026-07-06] Do NOT call `mqtt_client.disconnect()` or any PubSubClient method from a `WiFi.onEvent()` callback — it runs in lwIP context and causes a race condition. Set a `volatile bool` flag instead and handle it in `loop()`.

## Decision Log

<!-- Significant technical decisions with rationale. Why X was chosen over Y. -->

- [2026-07-06] setKeepAlive(30) chosen over 60 or 15: 30s balances fast drop detection (45s LWT) with low PING overhead. Default of 120s was causing 3-minute HA "unknown" windows.
