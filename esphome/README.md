# ESPHome rewrite — `esp32connector`

This directory replaces the Arduino firmware in `src/` for the device already
running ESPHome, reachable at **`esp32connector.local`** (mDNS — the LAN IP
is DHCP-assigned and can change, so don't hardcode it), managed by your Home
Assistant instance.

The Arduino code in `src/` and `include/` is kept as design reference but is
no longer the target build path.

---

## What changed

| Concern              | Arduino firmware                  | ESPHome rewrite                                |
|----------------------|-----------------------------------|------------------------------------------------|
| Calendar fetch       | `HTTPClient` + ArduinoJson v7     | HA `calendar.get_events` → template sensor → ESPHome `text_sensor` |
| Display              | `GxEPD2_3C<GxEPD2_750c>`          | `waveshare_epaper` (`model: 7.50in-bv3-bwr`)   |
| Web UI               | `ESPAsyncWebServer` dashboard     | Home Assistant (native API + `button` entity)  |
| Config storage       | NVS `Preferences`                 | `substitutions:` (recompile to change)         |
| OTA                  | `ArduinoOTA`                      | `ota:` (ESPHome dashboard, wireless)           |
| Time / DST           | `configTzTime`                    | `time.sntp` with POSIX TZ string               |

The visual layout is **800×480**:

- 568 px **5-week rolling month grid** (today always in row 3, −2 / +2 weeks
  visible) | 232 px **upcoming list** | 80 px **4-row footer**
- Red is accent only (header strip, today circle, event dots, sidebar date
  labels, month-boundary label inside cells)
- BWR has no partial refresh — every update is a full ~15 s repaint

The device also exposes a local web preview at `http://<device>/preview`
(and `/dash`) via the [`preview_page`](components/preview_page/) external
component — a JS-rendered SVG mirror of the e-paper layout, plus an info
card showing live wifi / RSSI / uptime / weather / sync status / people /
firmware version. Useful for iterating on layout without burning a refresh.

---

## Deployment

### 1. Wire up Google Calendar in Home Assistant

In HA: **Settings → Devices & Services → + Add Integration → Google Calendar**.
Follow the OAuth flow. HA will create one `calendar.*` entity per Google
calendar you allow it to see (e.g. `calendar.yourname_gmail_com`).

### 2. Add the template sensor + refresh script to HA

**Option A — YAML (shown below).** Add this to `/config/configuration.yaml`
(or split file) and reload YAML / restart HA. It queries three calendars at
once and merges them into a single JSON list sorted by start time. Adjust the
`entity_id:` list if you want different calendars.

**Option B — UI, no `configuration.yaml` editing.** Both pieces can also be
created entirely from the HA UI if you'd rather not hand-edit YAML files:
- Template sensor: **Settings → Devices & Services → Helpers → + Create
  Helper → Template → Template a sensor**, then switch the helper's editor to
  YAML mode (⋮ menu → **Edit in YAML**) and paste the `sensor:` block's
  contents below.
- Refresh script: **Settings → Automations & Scenes → Scripts → + Add
  Script**, then ⋮ menu → **Edit in YAML** and paste the `script:` block's
  contents below.

Either way you end up with the same `sensor.calendar_events_json` and
`script.refresh_home_calendar` entities that the device subscribes to.

```yaml
# /config/configuration.yaml

template:
  - trigger:
      - platform: time_pattern
        minutes: "/5"
      - platform: homeassistant
        event: start
      - platform: event
        event_type: home_calendar_refresh   # fired by script below
    action:
      - service: calendar.get_events
        target:
          entity_id:
            - calendar.yourname_gmail_com
            - calendar.family
            - calendar.work
        data:
          duration:
            days: 14
        response_variable: cal
    sensor:
      - name: "Calendar Events JSON"
        unique_id: calendar_events_json
        # State is just the count; the real payload lives in attributes.
        # cal is {"calendar.x": {"events": [...]}, "calendar.y": {"events": [...]}, ...}
        # We wrap the merged list in {"events": [...]} because the ESPHome
        # json::parse_json helper only accepts a JsonObject root.
        state: >-
          {{ cal.values() | map(attribute='events') | sum(start=[]) | count }}
        attributes:
          events: >-
            {{ {'events': cal.values()
                          | map(attribute='events')
                          | sum(start=[])
                          | sort(attribute='start')
                          | list} | tojson }}

script:
  refresh_home_calendar:
    alias: "Home Calendar: force refresh"
    sequence:
      - event: home_calendar_refresh
```

After reload you should see `sensor.calendar_events_json` with its state as
the total event count and an `events` attribute containing the merged JSON
array. If the entity id of the sensor or the script differs in your install,
update the `ha_events_entity` / `ha_refresh_script` substitutions at the top
of [esp32connector.yaml](esp32connector.yaml) to match.

### 3. Open the ESPHome dashboard

Home Assistant → **Settings → Add-ons → ESPHome → Open Web UI**.

If the device was flashed via the ESPHome dashboard *add-on* (as above), HA
picks it up automatically — no separate pairing step needed. If you're
instead pointing a standalone `esphome` CLI/dashboard at the device, or the
device's HA integration entry was ever deleted, pair it from the HA UI:
**Settings → Devices & Services → + Add Integration → ESPHome** — HA
discovers it via mDNS as `esp32connector`, or you can type the hostname/IP
and port `6053` manually. HA will prompt for the `api_encryption_key` (same
value as in `secrets.yaml`). This is entirely UI-driven — no
`configuration.yaml` edit required to add or re-add the device.

### 4. Replace the existing `esp32connector.yaml`

In the ESPHome dashboard, click your `esp32connector` device → **EDIT**.

- **First**, copy the existing file's contents to a scratchpad. You need to
  preserve at minimum:
  - the existing **`api: encryption: key:`** value (or that line in
    `secrets.yaml`) — losing this unlinks the device from HA
  - any sensors / switches / extra peripherals you already configured
- **Then** paste the contents of [esp32connector.yaml](esp32connector.yaml) here.
- Re-add anything from the scratchpad that this file doesn't already cover.

### 5. Ensure `secrets.yaml` has the keys

The yaml references three secrets — see [secrets.example.yaml](secrets.example.yaml).
The first two (`wifi_ssid`, `wifi_password`) almost certainly already exist in
your `/config/esphome/secrets.yaml`. The `api_encryption_key` needs to be the
*existing* key for this device (not a new one) so HA recognises it.

### 6a. First flash — USB cable (new device / blank board)

Use this path the **first time** this firmware is installed on a board, or
whenever the device is not reachable over WiFi.

1. Connect a USB-C cable to the board's **UART port** (the port that goes
   through the on-board CH343 USB-serial chip — check your board silkscreen;
   on the Heemol N16R8 this is the port closest to the 5V/GND end of the board).
2. In the ESPHome dashboard, open `esp32connector` → click **INSTALL**.
3. Choose **"Plug into this computer"** (not Wirelessly).
4. Select the serial port that appears — on macOS this shows up as
   `/dev/cu.usbmodem*` for the CH343 chip on this board (not
   `/dev/tty.usbserial-*`, which is the older CP210x/CH340 naming); on
   Windows it's `COMx`.
5. ESPHome compiles and flashes the firmware over the cable (~10–30 s).
6. On first boot, `improv_serial` advertises over the serial connection.
   The ESPHome dashboard detects this and prompts for your WiFi credentials.
   Alternatively the captive-portal AP (`esp32connector Setup`) is available
   if improv handshake is skipped.

> Keep the serial LOGS panel open during first boot to confirm the device
> connects to WiFi, syncs time, and receives the first calendar push.

### 6b. Subsequent flashes — OTA (device already on WiFi)

Once the device is online, all future firmware updates go wireless:

1. Edit the yaml in the ESPHome dashboard.
2. Click **SAVE → INSTALL → Wirelessly**.
3. ESPHome compiles inside the addon and OTAs to `esp32connector.local`.
   First compile of a fresh toolchain version pulls fonts and esp-idf
   (~5 min). Subsequent installs are fast (~1–2 min).

### 7. Verify

- ESPHome dashboard shows device green / online.
- HA → Devices → `esp32connector` shows a new `button.refresh_calendar` entity.
- Display does a 15–20 s full refresh and shows the current month with events.
- Status bar shows current `WiFi`, `IP`, the HA source sensor id, last sync
  time, status (`ok` / `parse err`), and event count.

---

## Things you'll likely need to tweak

All tunables live in `substitutions:` at the top of
[esp32connector.yaml](esp32connector.yaml).

| What                              | Where                                                    |
|-----------------------------------|----------------------------------------------------------|
| HA calendar sensor entity id      | `substitutions.ha_events_entity`                         |
| HA refresh script entity id       | `substitutions.ha_refresh_script`                        |
| Firmware version (footer + HA)    | `substitutions.sw_version`                               |
| People shown in footer (4 slots)  | `substitutions.person_{a,b,c,d}_entity` + `_label`       |
| Speed sensors (optional, 4 slots) | `substitutions.speed_{a,b,c,d}_entity`                   |
| Weather entity                    | `substitutions.weather_entity` (default `weather.forecast_home`) |
| Refresh cadence (default 5 min)   | HA template sensor `time_pattern` trigger (in HA config) |
| Timezone (currently CET/CEST)     | `substitutions.tz` (POSIX TZ string)                     |
| Display model (board rev)         | `display[0].model` — `7.50in-bv3-bwr` (V3, current), `7.50in-bv2` (V2)  |
| Cell-label truncation (9 chars)   | `label.resize(9)` in the display lambda                  |
| Sidebar title truncation (22)     | `t.resize(21)` in the display lambda                     |
| Sidebar max events (10)           | `shown < 10` check in the display lambda                 |
| Grid max events per cell (3 dots) | `dots < 3` check in the display lambda                   |

### Presence / speed sensors

Each person slot pairs a `person.*` entity (zone state) with an optional
`sensor.*_speed` entity. The HA Companion App **disables Speed by default**
— enable it per phone via Companion App → Manage Sensors → Speed. The
device tolerates missing / unavailable / unknown sensors and just omits the
`(NN km/h)` suffix until a real value (≥ 1 m/s, i.e. ≳ 3.6 km/h) arrives.
Non-Companion-App people (e.g. grandparents) can be fed via Life360 — just
point a slot's `person_X_entity` at the resulting `device_tracker.life360_*`.

---

## Trigger a manual refresh from HA

The yaml exposes a `button.refresh_calendar` entity. Pressing it (from the
device card, an automation, a dashboard tile, or `button.press` service)
calls the HA `script.refresh_home_calendar` script via the native API, which
fires the `home_calendar_refresh` event, which re-triggers the template
sensor, which pushes the fresh JSON to the device — which then forces a
display update without waiting for the 5-minute interval.

---

## Known caveats

1. **BWR model name.** This device uses a **V3** board, so
   [esp32connector.yaml](esp32connector.yaml) is set to `model: 7.50in-bv3-bwr`
   (three-colour V3, Waveshare part WS-13505 V3). If you're on the older V2
   board use `7.50in-bv2` instead (B/W/R, no `-bwr` suffix); using the wrong
   one for your hardware garbles the picture. Plain `7.50in-bv3` (no `-bwr`)
   is the V3 board's **black/white-only** variant — don't use it here, the
   panel is three-colour.
2. **No partial refresh.** Each update is a full 15–20 s repaint. Don't
   shorten the HA template-sensor cadence below ~2 min or you'll burn the panel.
3. **ESP32-S3 framework.** The yaml uses `esp-idf` (not Arduino) for stable,
  predictable behavior with `waveshare_epaper`, native API, and local web
  handlers on S3 boards.
4. **Fonts download at compile time.** First build needs internet access in
   the ESPHome addon container to pull Roboto from Google Fonts.
5. **No HA = no calendar.** Because the device subscribes to a HA sensor
   instead of polling Google directly, the panel stops updating if HA is
   offline. Last good render stays on the screen (BWR holds without power).
