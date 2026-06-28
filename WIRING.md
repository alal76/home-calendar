# Wiring — nanoESP32-C6

All connections use female-to-female dupont jumper wires (2.54mm).

**Schematic diagram:** [docs/wiring-diagram.svg](docs/wiring-diagram.svg) —
all signals colour-matched to the jumper-wire convention below.

## E-ink display (Waveshare 7.5" BWR HAT)

| HAT Pin | Signal | nanoESP32-C6 | Wire colour |
|---------|--------|--------------|-------------|
| VCC | 3.3V | **3V3** | Red |
| GND | Ground | **GND** | Black |
| DIN | MOSI | **GPIO7** | Blue |
| CLK | SPI clock | **GPIO6** | Yellow |
| CS | Chip select | **GPIO5** | Orange |
| DC | Data/command | **GPIO4** | Green |
| RST | Reset | **GPIO3** | Purple |
| BUSY | Busy | **GPIO2** | White |

## Manual refresh button

| Button | nanoESP32-C6 |
|--------|--------------|
| Leg 1 | **GPIO0** |
| Leg 2 | **GND** |

## Audio out — MAX98357A I2S DAC (optional)

| DAC Pin | nanoESP32-C6 | Wire colour |
|---------|--------------|-------------|
| VIN | **3V3** | Red |
| GND | **GND** | Black |
| BCLK | **GPIO18** | Blue |
| LRC | **GPIO19** | Yellow |
| DIN | **GPIO20** | Orange |

## Microphone — INMP441 I2S MEMS (optional)

| Mic Pin | nanoESP32-C6 | Wire colour |
|---------|--------------|-------------|
| VDD | **3V3** | Red |
| GND | **GND** | Black |
| SCK | **GPIO21** | Blue |
| WS | **GPIO22** | Yellow |
| SD | **GPIO23** | Orange |
| L/R | **GND** | Black |

## ⚠ Reserved pins — do NOT use

| GPIO | Reason |
|------|--------|
| GPIO8 | Onboard WS2812B RGB LED |
| GPIO9 | BOOT button |

## Free GPIOs for expansion

GPIO1, GPIO10, GPIO11, GPIO12, GPIO13, GPIO15, RX, TX

## Power

Single USB-C into the **native USB-C port** (bottom port) for wall power,
or use the **CH343 port** (top) which also powers the board during flashing.
Both ports provide 5V to the onboard regulator.
