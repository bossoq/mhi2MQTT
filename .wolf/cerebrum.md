# Cerebrum

> OpenWolf's learning memory. Updated automatically as the AI learns from interactions.
> Do not edit manually unless correcting an error.
> Last updated: 2026-06-30

## User Preferences

<!-- How the user likes things done. Code style, tools, patterns, communication. -->

## Key Learnings

- **The `Stop` hook fires at the end of every TURN, not once per session.** Any hook writing a "session summary" must be idempotent: guard on whether anything changed, replace prior records rather than appending, and apply deltas (not absolutes) to cumulative totals. `.wolf/hooks/stop.js` violated all three and silently inflated `token-ledger.json` ~15x before it was caught (bug-038). `_session.json` accumulates for the whole session and is never reset — treat its counters as cumulative, never as per-turn.

- **ArduinoJson is v7 (7.3.0), not v6.** `DynamicJsonDocument(capacity)` is deprecated in v7 and the capacity argument is **ignored** — the document grows on demand, `memoryUsage()` returns 0 and `overflowed()` is always false. Every `const size_t capacity*Config = JSON_OBJECT_SIZE(..) + ..` constant in `main.cpp` is therefore inert dead weight. Do NOT spend effort re-deriving those sizes when adding discovery keys; there is no overflow risk. (Verified 2026-09-28 by compiling a host-side replica against `.pio/libdeps/.../ArduinoJson`.)
- **HA MQTT discovery abbreviations for horizontal swing:** `swing_h_modes`, `swing_h_mode_cmd_t`, `swing_h_mode_stat_t`, `swing_h_mode_cmd_tpl`, `swing_h_mode_stat_tpl` → `swing_horizontal_mode_*`. Added to MQTT climate in HA core PR #139303 (HA 2025.3). Authoritative list: `homeassistant/components/mqtt/abbreviations.py`. **Field-verified working 2026-09-28** on the Bedroom_AC unit — the user's HA accepted the payload and the horizontal swing control appeared on the climate entity, so no version gate / settings toggle is needed for this deployment.
- **Discovery option lists must cover every value the mapper can emit.** `vaneLRToStr()` emits `SWING,WIDE,SPOT,1,2,3,4,5`; `vaneUDToStr()` emits `SWING,1,2,3,4`. Advertising a narrower list makes HA show the entity as `off`/`unknown` when an uncovered state arrives (see bug-034, bug-035).
- **Emit the literal `"None"` for an unknown MQTT state, never a guessed value.** HA's `mqtt/climate.py` `_handle_mode_received` maps `PAYLOAD_NONE` ("None", exact match) to `None`; `mqtt/select.py` matches `payload.lower() == "none"`. Both then show *unknown*. An **empty** render is NOT equivalent — climate logs "Invalid mode" and select silently ignores it, keeping a stale value. Using "None" needs no entry in `swing_modes`/`options`, so it adds no nonsense settable mode. Applied to `vaneUDToStr(SeeIRRemote)` (bug-036).
- **Any string that means "unknown" must round-trip back to a driver enum the setter refuses.** `strToVaneUD("None")` → `SeeIRRemote`, which `vanes_updown_set()` early-returns on. Without that it falls through `default:` → `Up`, so receiving the unknown value would silently command the vane upward.
- **`vanes_updown_get()` returns `SeeIRRemote` far more often than "edge case" suggests** — whenever MOSI DB0 `0x80` or DB1 `0x80` is clear, i.e. after every boot and after any IR remote use, until the ESP32 itself sets the vane. **Field-verified 2026-09-28** on Bedroom_AC: after an OTA reboot HA showed vertical swing as *unknown*, confirming the DB0/DB1 `0x80` set-bit semantics hold on real hardware.
- **OPEN (unverified): `vanes_leftright_get()` has the same blindness.** `vanes_leftright_set()` writes an "LR set" flag (`DB16 |= 0x10`) but the getter never checks it — it returns `DB16 & 0x07` unconditionally, so an IR-set horizontal position is reported as if known. `ACVanesLR` has no unknown member. Do NOT fix on inference: if MOSI `DB16 0x10` does not mean "position known", horizontal vane would report unknown permanently, regressing a confirmed-working feature. Verify by observing DB16 on hardware before/after IR remote use. Related: `DB16 & 0x07` can yield 7, undefined in the enum. Upstream context: lvschouwen/MHI-AC-Ctrl#39, #20.

### MQTT state publishing (2026-09-29)

- **The driver already keeps wanted and current state apart, at the SPI layer.** `SpiState` setters (`power_set`, `mode_set`, …) write the outbound **MISO** frame; the getters (`power_get`, …) decode the **MOSI** snapshot the A/C sends back. So a setter never changes what the getter returns — the A/C has to echo the command first. `pollMhiState()` overwrites `currentSettings` from MOSI on every loop iteration, which is why a command's value vanishes from `currentSettings` within milliseconds unless it is held somewhere else.
- **Hold commanded values per field, never with a blanket publish window.** `wantedSettings` + a `pendingFields` bitmask + `effectiveSettings()` (current, with unconfirmed fields overlaid) is the pattern. A per-field mask beats an all-fields comparison because some fields can never confirm — on short frames `vanes_leftright_get()` reads a DB16 that is not in the frame — and one such field would otherwise hold the whole confirmation open until the timeout.
- **Do not set a pending bit when the driver setter is a no-op**, or the value is held optimistically for the full timeout while nothing is sent: `vanes_updown_set(SeeIRRemote)` and `mode_set(mode_unknown)` both early-return.
- **`strToMode()` never returns `mode_unknown`** — every unrecognised string falls through to `mode_cool`. Validate mode payloads with an explicit whitelist before calling it; a `== mode_unknown` guard is dead code.
- **`rootInfo` is a shared global.** Anything publishing to `ha_state_topic` must rebuild it in full (`buildStateJson()`), never mutate a key or two in place — a partial document makes HA's value templates render those sensors unknown.
- **The A/C setpoint is quantized to half-degree Celsius steps** (`target_temp_encode` = `roundf(c*2)`, `target_temp_decode` = `db2/2`). A Fahrenheit setpoint never lands on one — 73 °F = 22.7778 °C comes back as 23.0 °C — so any confirm-by-comparison on temperature must round-trip the value through the codec first (`quantizeSetpoint()`), not widen an epsilon. Verified host-side against `mhi-frame.cpp`: 72/73/74/75 °F all fail a 0.05 epsilon.
- **Publish on change, with `update_int` as a heartbeat**, not the other way round. Compare the merged control state against what was last published; the heartbeat then only carries slow-moving telemetry. This also cuts IR-remote change latency from up to `update_int` down to one loop iteration.

### Versioning convention (2026-09-29)

- **Firmware versions follow the sibling mitsubishi2MQTT repo: `YYYY.M.N`**, N a per-month sequence (`2026.9.0`, `2026.9.1`, …), set in `config.h` (`mhi2mqtt_version` / `m2mqtt_version`). Commit subject is `Version <version> - <what changed>`. mhi2MQTT sat on a never-bumped `"1.0"` until 2026-09-29; bump it with any flashed change, or the device cannot be told apart from an older build after an OTA.
- **The version already reaches three places** with no extra wiring — web footer (`_VERSION_` in `html_common.h`), HA discovery device `sw` (`addMQTTDeviceInfo`), and the boot log. Only the constant needs changing.
- `bossoq/mitsubishi2MQTT` is the user's fork of the sibling project (its version string carries a `magi's edition` prefix; mhi2MQTT has no such branding and uses the bare date).

- **Project:** mhi2MQTT
- **Description:** Control your Mitsubishi Heavy Industries Air Conditioner locally with Home Assistant using ESP32. Communicates directly with A/C using SPI Communication via CNS port.

### MQTT / WiFi reliability (2026-07-06)

- **keepalive = 30s** is the correct value. The previous 120s gave a 180s LWT delay and 135s silent-drop detection window — both too slow for HA to reflect reality.
- **WiFi.onEvent callback is not PubSubClient-safe** — it runs in lwIP task context. Never call `mqtt_client` methods directly from it. Use a `volatile bool wifi_disconnected` flag and act on it in `loop()`.
- **Availability must be re-published periodically** (every 60s in `loop()`, inside MQTT-connected branch). Single publish on `mqttConnect()` is lost if broker restarts and clears retained messages. The payload must mirror `_debugMode`: `!_debugMode ? mqtt_payload_available : mqtt_payload_unavailable` — same as the connect-time publish.
- **`hpStatusChanged()` is called from `loop()`**, not directly from the MHI FreeRTOS task, so `mqtt_client.publish()` inside it is thread-safe. The FreeRTOS task only writes to the SPI snapshot under its own semaphore.

- **`http://<device>/api/logs` returns the whole serial log buffer over HTTP** — the same `[tag:<uptime_s>] ...` lines the USB monitor shows, including the full `Update State: {...}` JSON of every MQTT publish. This is the instrument for validating an OTA-flashed unit with no serial cable attached; the buffer is rolling (~800 lines, ≈2 min of traffic at default verbosity), so fetch it right after the event. **Do not enable `_debugMode` for this** — `hpPacketDebug()` publishes every SPI frame and floods the broker.
- **Validating optimistic publish without extra firmware logging:** a confirmed command is silent (the pending bit just clears), so absence of a message cannot prove success on its own. The falsifiable signal is the *revert*: if the A/C never echoes, `refreshPendingFields()` clears the bits at `COMMAND_CONFIRM_TIMEOUT_MS` and the next publish carries the pre-command value. Watching `/state` for >timeout+margin and seeing no flip-back therefore proves the echo landed inside the window. A/C-side action shows up independently in `fanRPM`/`compressorFrequency`, and an `operating` flip forces an extra publish a second or two after an OFF.

- **bug-042's `quantizeSetpoint` is hardware-confirmed (2026-09-29).** Commanding `temp/set 24.3` on Bedroom_AC publishes `temperature: 24.5` within 0.4 s and holds it past the confirm deadline — the A/C's half-degree echo matches the value we latched, so the pending bit clears normally. Note Bedroom_AC is configured in **Celsius**, so the Fahrenheit path that motivated the fix (73 F -> 22.78 C) is still untested on hardware; the off-grid Celsius value exercises the same rounding.

## Do-Not-Repeat

<!-- Mistakes made and corrected. Each entry prevents the same mistake recurring. -->
<!-- Format: [YYYY-MM-DD] Description of what went wrong and what to do instead. -->

- [2026-09-29] **Never propose flashing this firmware to Livingroom_AC (10.1.50.5) or Office_AC (10.1.50.6).** Those are a *different A/C brand* served by a different firmware repo; only **Bedroom_AC (10.1.50.7)** is an MHI unit running `mhi2MQTT`. After validating a change on Bedroom_AC there is no fleet to roll it out to — the deployment target list for this repo is exactly one device.

- [2026-09-29] Do NOT gate MQTT state publishes on a blanket "wait N seconds after any command" window (the old `POLL_DELAY_AFTER_SET_MS = 25000`). It blocks correct A/C state just as hard as stale state, and it was the cause of the 5–25 s lag in HA. Overlay the commanded value per field instead, and let every publish read the merged view.
- [2026-07-06] Do NOT call `mqtt_client.disconnect()` or any PubSubClient method from a `WiFi.onEvent()` callback — it runs in lwIP context and causes a race condition. Set a `volatile bool` flag instead and handle it in `loop()`.

## Decision Log

<!-- Significant technical decisions with rationale. Why X was chosen over Y. -->

- [2026-07-06] setKeepAlive(30) chosen over 60 or 15: 30s balances fast drop detection (45s LWT) with low PING overhead. Default of 120s was causing 3-minute HA "unknown" windows.
