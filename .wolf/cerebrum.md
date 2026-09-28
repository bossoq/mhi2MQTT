# Cerebrum

> OpenWolf's learning memory. Updated automatically as the AI learns from interactions.
> Do not edit manually unless correcting an error.
> Last updated: 2026-06-30

## User Preferences

<!-- How the user likes things done. Code style, tools, patterns, communication. -->

## Key Learnings

- **ArduinoJson is v7 (7.3.0), not v6.** `DynamicJsonDocument(capacity)` is deprecated in v7 and the capacity argument is **ignored** — the document grows on demand, `memoryUsage()` returns 0 and `overflowed()` is always false. Every `const size_t capacity*Config = JSON_OBJECT_SIZE(..) + ..` constant in `main.cpp` is therefore inert dead weight. Do NOT spend effort re-deriving those sizes when adding discovery keys; there is no overflow risk. (Verified 2026-09-28 by compiling a host-side replica against `.pio/libdeps/.../ArduinoJson`.)
- **HA MQTT discovery abbreviations for horizontal swing:** `swing_h_modes`, `swing_h_mode_cmd_t`, `swing_h_mode_stat_t`, `swing_h_mode_cmd_tpl`, `swing_h_mode_stat_tpl` → `swing_horizontal_mode_*`. Added to MQTT climate in HA core PR #139303 (HA 2025.3). Authoritative list: `homeassistant/components/mqtt/abbreviations.py`. **Field-verified working 2026-09-28** on the Bedroom_AC unit — the user's HA accepted the payload and the horizontal swing control appeared on the climate entity, so no version gate / settings toggle is needed for this deployment.
- **Discovery option lists must cover every value the mapper can emit.** `vaneLRToStr()` emits `SWING,WIDE,SPOT,1,2,3,4,5`; `vaneUDToStr()` emits `SWING,1,2,3,4`. Advertising a narrower list makes HA show the entity as `off`/`unknown` when an uncovered state arrives (see bug-034, bug-035).
- **Emit the literal `"None"` for an unknown MQTT state, never a guessed value.** HA's `mqtt/climate.py` `_handle_mode_received` maps `PAYLOAD_NONE` ("None", exact match) to `None`; `mqtt/select.py` matches `payload.lower() == "none"`. Both then show *unknown*. An **empty** render is NOT equivalent — climate logs "Invalid mode" and select silently ignores it, keeping a stale value. Using "None" needs no entry in `swing_modes`/`options`, so it adds no nonsense settable mode. Applied to `vaneUDToStr(SeeIRRemote)` (bug-036).
- **Any string that means "unknown" must round-trip back to a driver enum the setter refuses.** `strToVaneUD("None")` → `SeeIRRemote`, which `vanes_updown_set()` early-returns on. Without that it falls through `default:` → `Up`, so receiving the unknown value would silently command the vane upward.
- **`vanes_updown_get()` returns `SeeIRRemote` far more often than "edge case" suggests** — whenever MOSI DB0 `0x80` or DB1 `0x80` is clear, i.e. after every boot and after any IR remote use, until the ESP32 itself sets the vane. **Field-verified 2026-09-28** on Bedroom_AC: after an OTA reboot HA showed vertical swing as *unknown*, confirming the DB0/DB1 `0x80` set-bit semantics hold on real hardware.
- **OPEN (unverified): `vanes_leftright_get()` has the same blindness.** `vanes_leftright_set()` writes an "LR set" flag (`DB16 |= 0x10`) but the getter never checks it — it returns `DB16 & 0x07` unconditionally, so an IR-set horizontal position is reported as if known. `ACVanesLR` has no unknown member. Do NOT fix on inference: if MOSI `DB16 0x10` does not mean "position known", horizontal vane would report unknown permanently, regressing a confirmed-working feature. Verify by observing DB16 on hardware before/after IR remote use. Related: `DB16 & 0x07` can yield 7, undefined in the enum. Upstream context: lvschouwen/MHI-AC-Ctrl#39, #20.

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
