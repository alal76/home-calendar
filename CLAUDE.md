# CLAUDE.md

Persistent context for Claude Code in this repo. Keep this file and
[.github/copilot-instructions.md](.github/copilot-instructions.md) in sync —
they cover the same facts for different tools; when one changes, update the
other.

## Project

A wall-mounted home calendar display:
- **Device:** `esp32connector` — ESP32-S3-DevKitC-1 (Heemol N16R8 module
  with antenna interface). Reachable at **`esp32connector.local`** (mDNS).
  The LAN IP is DHCP-assigned and *will* change — never hardcode a numeric
  IP in docs, code, or examples; always use the hostname. It's derived from
  `substitutions.device_name` in the yaml, so renaming the device is a
  one-line change that updates the hostname everywhere.
- **Display:** Waveshare 7.5" e-Paper HAT (B), **V3** board, three-colour
  BWR, 800×480 (WS-13505 V3) — ESPHome model id `7.50in-bv3-bwr`. Don't
  confuse with plain `7.50in-bv3` (same board, B/W-only firmware variant) or
  `7.50in-bv2` (older V2 hardware) — wrong model id garbles the picture.
- **Firmware:** ESPHome YAML, compiled/flashed via the ESPHome add-on inside
  Home Assistant (not from this Mac — see Deployment below).
- **Data source:** Home Assistant's built-in Google Calendar integration. A
  HA template sensor (`sensor.calendar_events_json`) re-runs
  `calendar.get_events` every 5 min and exposes the merged events list as
  its `events` attribute; the device subscribes to that sensor over the
  ESPHome native API (encrypted). The ESP32 never talks to Google directly.
- **HA pairing:** UI-driven — HA **Settings → Devices & Services → + Add
  Integration → ESPHome** (mDNS discovery or manual host + port `6053`).
  Never suggest a YAML `esphome:` platform block in HA's
  `configuration.yaml` for this; that's the legacy pre-config-flow pattern.
- **Local debug UI:** the `preview_page` external component serves
  `GET /preview` and `/dash` on port 80 directly from the device — a
  JS-rendered SVG mirror of the e-paper layout plus a live stats card. This
  is separate from, and doesn't require, the HA connection.

The Arduino/PlatformIO firmware that used to live in `src/` was retired when
the device was adopted into ESPHome — don't suggest restoring it. An
OpenClaw skill on a Proxmox VM was briefly used as the calendar bridge and
has been fully removed — don't suggest reintroducing it. Both are preserved
only in git history (see [README.md § History](README.md#history)).

## Repository layout

```
home-calendar/
├── esphome/
│   ├── esp32connector.yaml          ← the device firmware (ESPHome YAML)
│   ├── components/preview_page/     ← local external_component (GET /preview, /dash)
│   ├── secrets.example.yaml         ← keys to add to /config/esphome/secrets.yaml
│   └── README.md                    ← deploy steps + HA template-sensor snippet
├── scripts/                         ← dev helpers (browser preview, etc.)
├── docs/                            ← wiring diagram SVGs
├── preview.html                     ← static preview snapshot (offline reference)
├── WIRING.md                        ← pin map + cabling reference
├── README.md                        ← project overview, architecture, quick start
└── .github/copilot-instructions.md  ← Copilot's mirror of this file
```

## Critical hardware constraints

- Display is **3-colour BWR, V3 board** — ESPHome model id `7.50in-bv3-bwr`.
- BWR has **no partial refresh**. Every update is a full 15–20 s repaint —
  expected behavior, never treat it as a bug. Don't shorten the HA
  template-sensor refresh cadence below ~2 min or you'll stress the panel.
- **GPIO0** is the BOOT strap pin, reused at runtime as the "reset to
  current month" button — holding it at power-on enters download mode; a
  brief press after boot is a normal input event. Don't repurpose it
  without accounting for that.
- **GPIO20/21** are native USB D−/D+ — avoid as general GPIO.
- **GPIO43/44** are UART0 TX/RX (USB-serial logging path) — leave free.
- **GPIO48** is the onboard RGB status LED on most S3 DevKitC-1 boards.
- **GPIO3** (display RST) is a strapping pin too, but it's driven as an
  MCU *output* to the panel (nothing drives back into it), so it doesn't
  interfere with boot strapping.
- Display VCC must be **3.3 V**, never 5 V.

## Pin assignments (ESP32-S3-DevKitC-1)

Wire by GPIO label on the board silkscreen — physical header numbering
varies by vendor/batch for S3 clones. Full rail-by-rail layout and the
color-coded wiring table live in [WIRING.md](WIRING.md) and
[docs/wiring-display-s3.svg](docs/wiring-display-s3.svg).

| Function | GPIO |
|----------|------|
| EPD MOSI/DIN | 7 |
| EPD CLK | 6 |
| EPD CS | 5 |
| EPD DC | 4 |
| EPD RST | 3 |
| EPD BUSY | 2 |
| EPD PWR (panel power enable, Rev 2.3+ HAT) | 1 |
| Button (BOOT / reset-to-current-month) | 0 |
| Prev / Next month buttons | 10 / 11 |
| I2S out (DAC) BCLK/LRC/DIN (optional) | 15/16/17 |
| I2S in (mic) SCK/WS/SD (optional) | 18/45/14 |

[docs/wiring-diagram-c6.svg](docs/wiring-diagram-c6.svg) is a **legacy**
ESP32-C6 diagram kept for historical reference only — that's a different
chip with a different pinout (e.g. it has GPIO22/23, which the S3 lacks).
Don't use it to answer questions about the current S3 hardware.

## ESPHome conventions

- Firmware: [esphome/esp32connector.yaml](esphome/esp32connector.yaml).
- `esp32: framework: type: esp-idf` — don't suggest switching to `arduino`.
- `logger: hardware_uart: UART0` routes serial output to GPIO43/44 (the
  CH343 USB-UART port). `improv_serial:` handles first-boot WiFi
  provisioning over that same serial link.
- All tunable knobs go in `substitutions:` at the top of the yaml —
  `sw_version`, the 4 person/speed entity pairs, weather entity, `tz`,
  calendar sensor / refresh script entity ids, `device_name` (drives the
  mDNS hostname).
- Calendar data arrives via `text_sensor: platform: homeassistant`
  subscribing to `sensor.calendar_events_json` (attribute `events`). The
  `on_value:` lambda calls `json::parse_json` to populate globals. There is
  no HTTP client / `interval:` poll on the device — HA pushes, the device
  doesn't pull.
- Parsed events are stashed in `globals:` (`ev_titles`, `ev_starts`,
  `ev_ends`, `ev_all_day`, `ev_count`) as fixed-size `std::array`s.
- Display rendering is one large lambda on the `display:` component.
  Layout: **568 px month grid | 232 px upcoming list | 80 px footer**.
- The grid is a **5-week rolling window** — today anchored in row 3, cells
  span −2 weeks to +2 weeks. `month_offset` (global int) lets the
  prev/today/next buttons shift the window in 4-week steps.
- Footer has 4 rows at `y = STATUS_Y + (8, 30, 52, 70)`: updated time +
  counts; next event; WiFi/IP/uptime + `${device_name} v${sw_version}`;
  4-person presence line.
- Red (`id(col_red)`) is accent-only: header strip, today circle, event
  dots, sidebar date labels, day-1 month label. Everything else is
  `id(col_black)` on white.
- Time via `time.sntp` with a POSIX TZ string (`substitutions.tz`) for
  automatic DST.

## Local `preview_page` external component

- [esphome/components/preview_page/preview_page.h](esphome/components/preview_page/preview_page.h)
  (+ [__init__.py](esphome/components/preview_page/__init__.py) for the
  ESPHome component registration).
- Adds `GET /preview` and `/dash` to the built-in `web_server`. Returns a
  self-contained HTML page: a JS-rendered SVG mirror of the e-paper layout
  plus an info card with live device stats.
- Receives pointers/refs to event globals + text_sensors via setters called
  from the `on_boot:` lambda in the yaml. Add new fields by extending the
  setters/private fields in the header, then wiring them in `on_boot:`.
- Uses ESPHome's `AsyncWebServerRequest::url_to(buf)` API — do **not**
  reintroduce the deprecated `request->url()` getter (removed in ESPHome
  2026.9).
- `js_escape()` inside the file handles inlining strings into the embedded
  `<script>` block.

## Deployment

- **First flash**: USB-C into the board's UART port (CH343 chip; on macOS
  this enumerates as `/dev/cu.usbmodem*`, not `/dev/tty.usbserial-*`) →
  ESPHome dashboard → INSTALL → **Plug into this computer**. `improv_serial`
  handles WiFi provisioning automatically.
- **Subsequent flashes**: ESPHome dashboard → INSTALL → **Wirelessly** (OTA
  to `esp32connector.local`).
- Don't suggest `esphome run`/`esphome compile` from the Mac, or anything
  PlatformIO/Arduino-related — compilation happens inside the HA ESPHome
  add-on.
- HA-side config (template sensor, refresh script) can be either YAML in
  `/config/configuration.yaml` or HA UI Helpers/Scripts — both are
  documented in
  [esphome/README.md](esphome/README.md#2-add-the-template-sensor--refresh-script-to-ha).
  It's not part of this repo either way.

## Debugging a "device won't boot / no data" report

Work through these in order — most such reports turn out to be a layer
*after* boot, not an actual boot failure:

1. **Serial boot log** — connect the UART port (`/dev/cu.usbmodem*` on
   macOS) at 115200 baud. Look for `setup() finished successfully!` in the
   ESPHome log lines; if present, firmware boot is fine and the issue is
   elsewhere.
2. **Network reachability** — `ping esp32connector.local` and
   `curl http://esp32connector.local/preview`. Both succeeding confirms
   WiFi and the device's own web server, independent of Home Assistant.
3. **Native API port** — `nc -z esp32connector.local 6053` confirms the
   ESPHome API server is listening.
4. **Home Assistant link** — if 1–3 all pass but HA shows the device's
   entities as `unavailable`, HA's connection to the device (not the device
   itself) is the problem. Check an entity's `last_changed` via HA's REST
   API (`GET /api/states/<entity_id>`, long-lived access token) — a
   long-stale `unavailable` timestamp means HA lost/never re-established
   the API link, usually a cached stale IP or a changed
   `api_encryption_key`. Fix via HA → Settings → Devices & Services →
   ESPHome → the device entry → **Reload** (or delete/re-add, see
   [esphome/README.md](esphome/README.md)).

## When suggesting changes

- All code changes are YAML + ESPHome lambda C++ (a restricted subset — no
  arbitrary headers; use what ESPHome exposes via `id()` and helper APIs).
- For new entities exposed to Home Assistant, prefer standard ESPHome
  platforms (`sensor`, `text_sensor`, `binary_sensor`, `button`, `switch`,
  `number`, `select`) over custom components.
- HA-side configuration lives on the HA box, not in this repo — reference
  it, don't try to edit it from here.
- Keep IP addresses out of docs/code — use `esp32connector.local` (or
  `substitutions.device_name` + `.local`) everywhere a device address is
  needed.
