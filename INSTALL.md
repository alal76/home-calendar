# Installation & Build Guide

A complete, start-to-finish path for building your own Home Calendar: parts
list, wiring, Home Assistant setup, firmware configuration, and first flash.
Every step below is written to be **customer/household-neutral** — there are
no hardcoded names, IPs, or accounts anywhere in this repo; you supply your
own at the points marked below.

If you just want the short version, see [README.md § Quick
start](README.md#quick-start). This guide is the long version, with no
steps skipped.

---

## 1. What you need

### Hardware

| Part | Notes |
|------|-------|
| ESP32-S3 dev board | This project targets an **ESP32-S3-DevKitC-1** class board (16 MB flash / 8 MB PSRAM recommended, e.g. "N16R8"). Any ESP32-S3 board with enough free GPIOs works; adjust `substitutions.board_type` / `board_variant` in the yaml if yours differs. |
| Waveshare 7.5" e-Paper HAT (B), **V3**, three-colour (BWR), 800×480 | Part WS-13505 V3. The V2 board also works — see [esphome/README.md § Known caveats](esphome/README.md#known-caveats) for the model-id difference. |
| USB-C cable | For first flash + power. |
| 9× female-to-female 2.54 mm dupont jumper wires | No soldering required for the display. |
| *(Optional)* MAX98357A I²S DAC + 4–8 Ω speaker | Audio output. |
| *(Optional)* INMP441 I²S MEMS microphone | Audio input. |
| *(Optional)* 2× momentary push buttons | Prev/next month — the device already has a "reset to current month" button built in (the board's BOOT button). |

### Software / accounts

- A running **Home Assistant** instance (Core, OS, Supervised, or Container
  — anything with the ability to install the ESPHome add-on, or run a
  standalone `esphome` dashboard reachable from HA).
- The **ESPHome** add-on (or a standalone ESPHome dashboard) with network
  access to your ESP32-S3.
- A **Google account** with at least one calendar you want displayed.
- This repository, cloned or downloaded locally (only needed to read/copy
  [esphome/esp32connector.yaml](esphome/esp32connector.yaml) into the
  ESPHome dashboard — no local build toolchain is required).

---

## 2. Wire the hardware

Full pin tables, wire colours, and an annotated diagram are in
[WIRING.md](WIRING.md) and [docs/wiring-display-s3.svg](docs/wiring-display-s3.svg).
Summary for the required display connection:

| HAT pin | Signal | ESP32-S3 GPIO |
|---------|--------|---------------|
| VCC | 3.3 V | 3V3 |
| GND | Ground | GND |
| DIN | MOSI | GPIO7 |
| CLK | SPI clock | GPIO6 |
| CS | Chip select | GPIO5 |
| DC | Data/command | GPIO4 |
| RST | Reset | GPIO3 |
| BUSY | Busy | GPIO2 |
| PWR *(Rev 2.3+ HAT only)* | Panel power enable | GPIO1 |

> ⚠️ VCC **must** be 3.3 V — never 5 V.

Optional audio peripherals' wiring is also in [WIRING.md](WIRING.md). The
manual refresh / month-navigation buttons need no extra wiring by default —
the board's own BOOT button doubles as "reset to current month"
(GPIO0), and GPIO10/GPIO11 are free if you want physical prev/next buttons.

---

## 3. Set up Home Assistant

### 3.1 Install the ESPHome add-on (if you don't already have it)

HA → **Settings → Add-ons → Add-on Store** → search **ESPHome** → **Install**
→ **Start**. (If you're managing ESPHome outside HA — e.g. a standalone
`esphome` CLI/dashboard on another machine — that works too, as long as it
can reach the board over WiFi/USB and HA can reach the board's native API
port `6053`.)

### 3.2 Add the Google Calendar integration

HA → **Settings → Devices & Services → + Add Integration → Google
Calendar**. Follow the OAuth flow and authorize whichever calendar(s) you
want the device to show. This creates one `calendar.*` entity per calendar
(e.g. `calendar.yourname_gmail_com`, `calendar.family`).

### 3.3 Add the template sensor + refresh script

The device doesn't call Google directly — it subscribes to a small HA
template sensor that merges your chosen calendars into one JSON list. Full
YAML snippet, plus a no-YAML UI alternative (HA Helpers + Scripts), is in
[esphome/README.md § 2](esphome/README.md#2-add-the-template-sensor--refresh-script-to-ha).
**Edit the `entity_id:` list in that snippet to your own `calendar.*`
entities from step 3.2** — the ones in the example are placeholders.

After adding it, confirm in **Developer Tools → States** that
`sensor.calendar_events_json` exists with a populated `events` attribute.

### 3.4 (Optional) Presence tracking

If you want the footer to show household members' locations, HA needs
`person.*` entities for them (**Settings → People**) — and optionally
`sensor.*_speed` from the HA Companion App (disabled by default; enable
under Companion App → Manage Sensors → Speed on each phone) or from
Life360's `device_tracker.life360_*` for people without the Companion App.
You'll point the firmware at these entity ids in step 4.

---

## 4. Configure the firmware for your household

Open [esphome/esp32connector.yaml](esphome/esp32connector.yaml) and edit the
**`substitutions:`** block at the top — this is the only section you need to
touch for a standard install. Everything below is a knob, not a code change:

| Substitution | What to set it to |
|---|---|
| `device_name` | A unique hostname, e.g. `home-calendar`. This also becomes the mDNS address `<device_name>.local` — used everywhere instead of a numeric IP, since DHCP-assigned IPs change. |
| `friendly_name` | Display name shown in HA. |
| `ha_events_entity` / `ha_refresh_script` | Entity ids from steps 3.3 (defaults already match the example snippet — only change these if you used different names). |
| `tz` | Your POSIX TZ string (handles DST automatically). |
| `weather_entity` | Your HA weather entity, e.g. `weather.forecast_home` (HA's Met.no default) or your own. |
| `person_a_entity` … `person_d_entity` | Your `person.*` entities from step 3.4. Leave slots pointing at a nonexistent entity if you have fewer than 4 people — the device tolerates unavailable entities and just omits that footer segment. |
| `person_a_label` … `person_d_label` | Display names for the footer. |
| `speed_a_entity` … `speed_d_entity` | Optional `sensor.*_speed` entities from step 3.4. |
| `sw_version` | Bump this with every change you make — shown in HA and on the device footer. |

You do **not** need to touch GPIO pin numbers, the display lambda, or
anything below the `substitutions:` block unless you're changing hardware or
layout — see [esphome/README.md § Things you'll likely need to
tweak](esphome/README.md#things-youll-likely-need-to-tweak) for the full list
of what's safe to customize further.

### Secrets

Copy [esphome/secrets.example.yaml](esphome/secrets.example.yaml) to
`secrets.yaml` in the same directory as your ESPHome config (in the HA
ESPHome add-on this is `/config/esphome/secrets.yaml`) and fill in:

```yaml
wifi_ssid: "your-wifi-name"
wifi_password: "your-wifi-password"
api_encryption_key: "..."   # ESPHome dashboard generates this for you
ota_password: "..."         # ESPHome dashboard generates this for you
```

**Never commit `secrets.yaml`** — it's already gitignored. Only
`secrets.example.yaml` (placeholders) is meant to be tracked.

---

## 5. First flash (USB)

1. Connect USB-C to the board's **UART port** (the port routed through the
   on-board USB-serial chip — check your board's silkscreen if it has two
   USB-C ports). On macOS this enumerates as `/dev/cu.usbmodem*`.
2. In the ESPHome dashboard, click **+ New Device**, choose a manual/blank
   config (or import), then replace its contents with
   [esp32connector.yaml](esphome/esp32connector.yaml) (after you've edited
   the substitutions in step 4).
3. Click **INSTALL** → **Plug into this computer** → select the serial port.
4. ESPHome compiles and flashes over the cable. First build pulls fonts and
   the esp-idf toolchain (needs internet access from wherever ESPHome
   compiles — a few minutes); later builds are much faster.
5. On first boot, `improv_serial` advertises over the same serial
   connection and the ESPHome dashboard prompts for your WiFi credentials.
   If that handshake is missed, connect to the `<device_name> Setup`
   captive-portal WiFi network the device broadcasts instead.
6. Keep the dashboard's **LOGS** panel open to confirm the device connects
   to WiFi, syncs time, and does its first display refresh.

---

## 6. Pair the device with Home Assistant

If you flashed via the HA-hosted ESPHome add-on (step 5), HA usually
discovers the device automatically. If not — fresh HA instance, or you're
running ESPHome standalone — pair it manually from the HA UI:

**Settings → Devices & Services → + Add Integration → ESPHome** — HA
discovers it via mDNS using the `device_name` you set, or you can type the
hostname/IP and port `6053` yourself. You'll be prompted for the
`api_encryption_key` from your `secrets.yaml`. This is entirely UI-driven —
no `configuration.yaml` editing is needed to add the device.

---

## 7. Verify

- ESPHome dashboard shows the device green/online.
- HA → **Settings → Devices & Services → ESPHome** → your device shows
  entities including `button.refresh_calendar`.
- The panel does a ~15–20 s full refresh and shows the current month with
  your events.
- Visit `http://<device_name>.local/preview` in a browser for a live
  JS-rendered mirror of the e-paper layout plus a device-stats card — useful
  for checking data without waiting on a physical repaint.
- Footer shows WiFi/uptime info and your configured `sw_version`.

If the device seems unresponsive, work through the diagnostic steps in
[CLAUDE.md § Debugging a "device won't boot / no data"
report](CLAUDE.md#debugging-a-device-wont-boot--no-data-report) — serial log
→ network reachability → API port → HA entity state, in that order.

---

## 8. Ongoing updates

Once the device is on WiFi, all further changes are wireless:

1. Edit the yaml (layout, substitutions, whatever) in the ESPHome dashboard.
2. **SAVE → INSTALL → Wirelessly** — OTAs to `<device_name>.local`.
3. Bump `substitutions.sw_version` so you can confirm which build is running
   from the footer / HA diagnostic sensor.

To force an immediate calendar refresh without waiting for the 5-minute HA
cadence, press `button.refresh_calendar` from the device's HA card, a
Lovelace tile, or a service call.

---

## Troubleshooting

See [CLAUDE.md § Debugging a "device won't boot / no data"
report](CLAUDE.md#debugging-a-device-wont-boot--no-data-report) for a
step-by-step runbook (serial log, mDNS/HTTP reachability, native API port,
HA entity state) and [esphome/README.md § Known
caveats](esphome/README.md#known-caveats) for the most common gotchas (wrong
display model id, refresh cadence too aggressive for a no-partial-refresh
panel, missing internet access during first compile).
