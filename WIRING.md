# Wiring — ESP32-S3-DevKitC-1 (Heemol N16R8)

Board: Heemol ESP32-S3 N16R8 DevKitC-1 module (ESP32-S3-WROOM class,
with antenna interface). Pin labels and physical header numbering vary
by vendor; wire by **GPIO name on the board silkscreen**, not by
connector index from older C6 diagrams.

## Board layout from your labeled image

Orientation used below: USB connectors at the **bottom** (same orientation
as your second labeled image).

Left rail (top -> bottom, exactly as labeled):
`3V3, 3V3, RST, 4, 5, 6, 7, 15, 16, 17, 18, 8, 3, 46, 9, 10, 11, 12, 13, 14, 5Vin, GND`

Right rail (top -> bottom, exactly as labeled):
`GND, 43, 44, 1, 2, 42, 41, 40, 39, 38, 37, 36, 35, 0, 45, 48, 47, 21, 20, 19, GND, GND`

If your received batch has minor label differences, trust the silkscreen
on your board first.

All connections use female-to-female dupont jumper wires (2.54mm).

**Schematic diagrams:**
- [docs/wiring-display-s3.svg](docs/wiring-display-s3.svg) — S3 display wiring
  reference (ESP32-S3 ↔ Waveshare HAT).
- [docs/wiring-diagram-c6.svg](docs/wiring-diagram-c6.svg) — legacy C6 full
  wiring diagram kept for reference.

For this board revision, the rail lists above are authoritative for pin
locations.

All signals are colour-matched to the jumper-wire convention below.

## E-ink display (Waveshare 7.5" BWR HAT)

Table order matches the HAT-side JST connector top-to-bottom, so you can
read straight down the photo of the driver board and match
colour → pin. (Older 8-pin Rev 2.2 HATs omit `PWR` — just skip that row.)

| # | HAT Pin | Wire colour | ESP32-S3 GPIO | Board side | Signal |
|---|---------|-------------|----------------|------------|--------|
| 1 | PWR  | Brown  | **GPIO1** | Right rail | Panel power enable |
| 2 | BUSY | White  | **GPIO2** | Right rail | Busy |
| 3 | RST  | Purple | **GPIO3** | Left rail | Reset |
| 4 | DC   | Green  | **GPIO4** | Left rail | Data/command |
| 5 | CS   | Orange | **GPIO5** | Left rail | Chip select |
| 6 | CLK  | Yellow | **GPIO6** | Left rail | SPI clock |
| 7 | DIN  | Blue   | **GPIO7** | Left rail | MOSI |
| 8 | GND  | Black  | **GND**   | Either rail | Ground |
| 9 | VCC  | Red    | **3V3**   | Left rail | 3.3 V |

Photo reference: the driver board (Waveshare e-Paper Driver HAT Rev 2.3)
has the 9-pin JST connector on its top edge. Silk-screened labels next
to each pin read `PWR / BUSY / RST / DC / CS / CLK / DIN / GND / VCC`
top-to-bottom. The colour column above matches the ribbon shipped with
that HAT — brown at the PWR (top) end, red at the VCC (bottom) end.

`PWR` is present on Rev 2.3+ driver boards. Wire it to a free GPIO
(GPIO1 by convention) and pin it HIGH with a `switch: platform: gpio`
block using `restore_mode: ALWAYS_ON`. Tying directly to 3V3 also works,
but the GPIO route is cleaner and gives you an optional HA power-cycle
toggle.

## Manual refresh button

No external wiring needed by default: use the on-board **BOOT** button
(`GPIO0`). The YAML already treats it as a runtime button.

`GPIO0` is also the BOOT strap. Holding BOOT while resetting enters
download mode; a normal BOOT press after boot is a regular input event.

## Audio out — MAX98357A I2S DAC (optional)

Recommended S3 mapping (avoids USB data pins 19/20):

| DAC Pin | ESP32-S3 | Wire colour |
|---------|----------|-------------|
| VIN | **3V3** | Red |
| GND | **GND** | Black |
| BCLK | **GPIO15** | Blue |
| LRC | **GPIO16** | Yellow |
| DIN | **GPIO17** | Orange |

## Microphone — INMP441 I2S MEMS (optional)

Recommended S3 mapping (GPIO22/23 are not available on ESP32-S3):

| Mic Pin | ESP32-S3 | Wire colour |
|---------|----------|-------------|
| VDD | **3V3** | Red |
| GND | **GND** | Black |
| SCK | **GPIO18** | Blue |
| WS | **GPIO45** | Yellow |
| SD | **GPIO14** | Orange |
| L/R | **GND** | Black |

## ⚠ Reserved pins — do NOT use

| GPIO | Reason |
|------|--------|
| GPIO0 | BOOT strap button (on-board) |
| GPIO20 | Native USB D− |
| GPIO21 | Native USB D+ |
| GPIO43 | UART0 TX — USB-serial / first-flash path |
| GPIO44 | UART0 RX — USB-serial / first-flash path |
| GPIO48 | Onboard RGB LED |
| GPIO39 | MICK — board-level mic clock signal |
| GPIO40/41/42 | JTAG MTDO/MTDI/MTMS (usable as GPIO but avoid for clarity) |

## Free GPIOs for expansion

Commonly usable on S3 with this project wiring:
GPIO8, GPIO9, GPIO10 (prev-month btn), GPIO11 (next-month btn),
GPIO12, GPIO13, GPIO14 (mic SD), GPIO15 (DAC BCLK), GPIO16 (DAC LRC),
GPIO17 (DAC DIN), GPIO18 (mic SCK), GPIO19, GPIO35, GPIO36, GPIO37,
GPIO38, GPIO45 (mic WS), GPIO46, GPIO47.

## Power

Power the board over USB and let the onboard regulator provide 3.3 V to
the display and sensors. Do not back-feed 3V3 while USB is connected.
