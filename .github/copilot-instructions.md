# GitHub Copilot — Project Instructions

Persistent context for the home-calendar project. Copilot reads
`.github/copilot-instructions.md` automatically in VS Code.

## Project

A wall-mounted home calendar display:
- **Device:** `esp32connector` at **esp32connector.local** — ESP32-S3-DevKitC-1
  (Heemol N16R8 module with antenna interface)
- **Display:** Waveshare 7.5" e-Paper HAT (B) — BWR 800×480 (WS-13505)
- **Firmware:** **ESPHome YAML** managed by Home Assistant at **your-home-assistant-host**
- **Data source:** Home Assistant's built-in **Google Calendar** integration.
  A HA template sensor (`sensor.calendar_events_json`) re-runs
  `calendar.get_events` every 5 min and exposes the events list as the
  `events` attribute; the device subscribes via the ESPHome native API.
- **Integration UI:** Home Assistant (no separate web dashboard on the device)

The Arduino/PlatformIO firmware that previously lived in `src/` was retired
when the device was adopted into ESPHome — do not suggest restoring it. An
OpenClaw skill on a Proxmox VM was briefly used as the calendar bridge and
has been fully removed — do not suggest reintroducing it.

## Critical hardware constraints

- The display is **3-colour BWR** — ESPHome model id is `7.50in-bv2`
  (or `7.50in-bv3` for the newer V3 board).
- BWR has **no partial refresh**. Every update is a full 15–20 s repaint —
  this is expected, never treat it as a bug.
- **GPIO0** is the BOOT strap button — avoid assigning as a normal GPIO unless intentional.
- **GPIO20/21** are native USB D−/D+ — avoid assigning as general GPIO.
- **GPIO43/44** are UART0 TX/RX (USB-serial path) — leave free for logging.
- **GPIO48** is onboard RGB LED on most S3 DevKitC-1 boards.
- Display VCC must be **3.3 V**, never 5 V.

## Pin assignments (ESP32-S3-DevKitC-1)

Wire by GPIO labels on the board silkscreen. Physical header numbering
varies by vendor for S3 clones.

| Function | GPIO |
|----------|------|
| EPD MOSI/DIN | 7 |
| EPD CLK | 6 |
| EPD CS | 5 |
| EPD DC | 4 |
| EPD RST | 3 |
| EPD BUSY | 2 |
| Button | 0 |
| I2S out (DAC) BCLK/LRC/DIN | 15/16/17 |
| I2S in (mic) SCK/WS/SD | 18/45/14 |

## ESPHome conventions

- The firmware lives in [esphome/esp32connector.yaml](../esphome/esp32connector.yaml).
- ESP32-S3 configuration in this project uses `framework: type: esp-idf`.
  Don't suggest switching to `arduino` for this device.
- `logger: hardware_uart: UART0` routes serial output to GPIO43/44 for the
  USB UART port. `improv_serial:` is included for first-boot WiFi provisioning.
- All tunable knobs go in `substitutions:` at the top of the yaml —
  including `sw_version`, the 4 person/speed entity pairs, weather entity,
  TZ, calendar sensor / refresh script entity ids.
- Calendar data arrives via `text_sensor: platform: homeassistant` subscribing
  to `sensor.calendar_events_json` (attribute `events`). The `on_value:`
  lambda calls `json::parse_json` to populate globals. There is no HTTP
  client / `interval:` poll on the device.
- Parsed events are stashed in `globals:` (`ev_titles`, `ev_starts`,
  `ev_ends`, `ev_all_day`, `ev_count`) as fixed-size `std::array`s.
- Display rendering is one large lambda on the `display:` component. Layout
  is **568 px month grid | 232 px upcoming list | 80 px footer**.
- The grid is a **5-week rolling window** — today is anchored in row 3,
  cells span −2 weeks back to +2 weeks forward. A `month_offset` int
  global lets the prev/today/next buttons shift the window by 4-week steps.
- The footer has **4 rows** at y = STATUS_Y + (8, 30, 52, 70):
  1. Updated time + past/today/upcoming counts
  2. Next event (start, in Xd, title) — red if today
  3. WiFi + IP + uptime on the left, `${device_name} v${sw_version}` right
  4. People line: `Label: zone (NN km/h)` for each of the 4 slots
- Red (`id(col_red)`) is for accents only: header strip, today circle,
  event dots, sidebar date labels, day-1 month label. Everything else
  `id(col_black)` on the default white background.
- Time uses `time.sntp` with a POSIX TZ string (currently
  `CET-1CEST,M3.5.0,M10.5.0/3`) for automatic DST.
- The ESP32 never talks to Google directly — it only talks to Home
  Assistant over the encrypted native API.

## Local `preview_page` external component

- Lives in [esphome/components/preview_page/preview_page.h](../esphome/components/preview_page/preview_page.h).
- Adds `GET /preview` (and `/dash`) handlers to the built-in `web_server`.
  Returns a single self-contained HTML page that draws a JS-rendered SVG
  mirror of the e-paper layout and an info card with live device stats.
- Receives pointers / refs to event globals + text_sensors via setters
  called from the `on_boot` lambda. Add new fields by extending the
  setters/private fields in the .h, then wiring them in `on_boot`.
- Uses ESPHome's `AsyncWebServerRequest::url_to(buf)` API — do **not**
  re-introduce the deprecated `request->url()` getter (removed in
  ESPHome 2026.9).
- `js_escape()` helper inside the file handles inlining strings into the
  embedded `<script>` block.

## Deployment

- **First flash**: connect USB-C to the board's UART port → ESPHome dashboard
  → INSTALL → **Plug into this computer** → select the serial port.
  `improv_serial` handles WiFi provisioning automatically on first boot.
- **Subsequent flashes**: ESPHome dashboard → INSTALL → **Wirelessly** (OTA).
- Don't suggest `esphome run`, `esphome compile` from the Mac,
  or anything PlatformIO/Arduino-related.

## When suggesting changes

- All code changes are YAML + ESPHome lambda C++ (a restricted subset — no
  arbitrary headers; use what ESPHome exposes via `id()` and helper APIs).
- For new entities exposed to Home Assistant, prefer the standard ESPHome
  platforms (`sensor`, `text_sensor`, `binary_sensor`, `button`, `switch`,
  `number`, `select`) over custom components.
- HA-side configuration (template sensor, scripts, automations) lives in
  `/config/configuration.yaml` on the HA box, not in this repo. Reference
  it but don't try to edit it from here.
