# Home Calendar — ESPHome on nanoESP32-C6

A wall-mounted Google Calendar display on a Waveshare 7.5" black/white/red
e-ink panel, driven by a nanoESP32-C6 running **ESPHome** under Home
Assistant. Home Assistant's built-in Google Calendar integration does the
OAuth and event fetching; a template sensor pushes the events list to the
device over the native API.

```
Google Calendar ──► Home Assistant (your-home-assistant-host)
                       │  • Google Calendar integration  (OAuth + polling)
                       │  • template sensor every 5 min
                       │      sensor.calendar_events_json  (attribute: events)
                       ▼
                  ESPHome native API
                       │
                       ▼
                  esp32connector (esp32connector.local)  ──►  7.5" BWR panel
                                                       renders 800×480 layout
```

The ESP32 never talks to Google directly — only to Home Assistant.

---

## Repository layout

```
home-calendar/
├── esphome/
│   ├── esp32connector.yaml      ← the device firmware (ESPHome YAML)
│   ├── secrets.example.yaml     ← keys to add to /config/esphome/secrets.yaml
│   └── README.md                ← deploy steps + HA template-sensor snippet
├── WIRING.md                    ← pin map + cabling reference
└── .github/
    └── copilot-instructions.md  ← persistent Copilot context
```

---

## Quick start

### 1. Home Assistant (one time)

1. **Add the Google Calendar integration**
   HA → **Settings → Devices & Services → + Add Integration → Google Calendar**.
   Walk through the OAuth flow; HA creates one `calendar.*` entity per
   calendar you authorise.
2. **Add the template sensor + refresh script** to `/config/configuration.yaml`
   so HA exposes the events as JSON for the device to subscribe to.
   Full snippet is in [esphome/README.md](esphome/README.md#2-add-the-template-sensor--refresh-script-to-ha).
3. Restart HA (or call **Developer Tools → YAML → Reload Template Entities**)
   and confirm `sensor.calendar_events_json` exists with an `events` attribute
   containing a JSON array.

### 2. ESPHome device

The device `esp32connector` already exists in Home Assistant. To install
this calendar firmware, follow [esphome/README.md](esphome/README.md):

1. ESPHome dashboard → `esp32connector` → **EDIT**
2. Preserve the existing `api: encryption: key:` value
3. Paste the contents of [esphome/esp32connector.yaml](esphome/esp32connector.yaml)
4. **INSTALL → Wirelessly** — OTAs to esp32connector.local

---

## How it works

| Layer       | Component                                                       |
|-------------|-----------------------------------------------------------------|
| Calendar    | Home Assistant Google Calendar integration (OAuth + token refresh) |
| Bridge      | HA template sensor `sensor.calendar_events_json` (5-min trigger) re-runs `calendar.get_events` and stashes the result in an `events` attribute |
| Transport   | ESPHome native API (encrypted) — device subscribes to the sensor attribute |
| Device      | nanoESP32-C6 + Waveshare 7.5" HAT (B), pins in [WIRING.md](WIRING.md) |
| Firmware    | ESPHome YAML + lambdas (see [esphome/esp32connector.yaml](esphome/esp32connector.yaml)) |
| Refresh     | 5-min HA cadence + on-demand `button.refresh_calendar` + full repaint (~15 s) |

---

## Updating the calendar layout

All rendering lives in the `display:` lambda in [esphome/esp32connector.yaml](esphome/esp32connector.yaml).
Edit it in the ESPHome dashboard, click **INSTALL** to OTA the change.

Common tweaks are listed in the "Things you'll likely need to tweak" table in
[esphome/README.md](esphome/README.md).

---

## Forcing an immediate refresh

The yaml exposes `button.refresh_calendar` in Home Assistant. Press it from
the device card, from a Lovelace tile, or via service call `button.press` —
the device calls HA's `script.refresh_home_calendar`, which re-fires the
template-sensor trigger, which pushes a fresh events list to the device,
which forces a display repaint without waiting for the 5-min cadence.

---

## History

This project started as Arduino firmware built with pioarduino + GxEPD2 +
ESPAsyncWebServer. After the device was adopted into Home Assistant via
ESPHome, the entire C++ codebase was retired in favour of the ESPHome YAML
above. An OpenClaw skill on a Proxmox VM briefly handled the Google
OAuth before being replaced by Home Assistant's native Google Calendar
integration. The Arduino code and OpenClaw scaffolding are preserved only in
git history.
