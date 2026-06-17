# Astrobot — Wiring Guide

## Overview

Astrobot is an animated eye/display bot built on an **Inland Nano** (Arduino Nano
clone, ATmega328P). It shows expressive pixel-art eyes, a digital clock, and
reacts to vibration with a heart animation.

## Bill of Materials

| Component | Description |
|-----------|-------------|
| Inland Nano | Arduino Nano-compatible dev board (ATmega328P @ 16MHz) |
| HW-084 | DS1307 RTC module with coin cell backup (I2C) |
| 1.8" TFT LCD | 128×160 RGB display, ST7735 driver, 8-pin header |
| Vibration sensor | SW-420 or similar — digital/analog vibration switch |
| 8Ω 0.25W speaker | Adafruit "Thin Plastic Speaker w/Wires" — plays melody on vibration |
| 100Ω resistor | Current-limiter for speaker (protects Nano pin and speaker coil) |
| USB power cable | Micro-USB for Nano |
| Hookup wire | Female-to-female dupont, ~12 wires |
| 3D printed enclosure | Custom housing for all components |

### TFT module pinout (label order on module)

```
GND VCC SCL SDA RES DC CS BL
```

## Wiring Table

### TFT LCD (to Nano)

| TFT Module | Nano Pin | Cable Color |
|------------|----------|-------------|
| GND | GND | Black |
| VCC | 3.3V *(use 3.3V, not 5V, for the ST7735)* | Red |
| SCL | A1 | Yellow |
| SDA | A2 | Green |
| RES | D5 | Blue |
| DC | A3 | Orange |
| CS | D2 | Purple |
| BL | A0 | White |

### HW-084 RTC (DS1307, to Nano)

| HW-084 | Nano Pin | Cable Color |
|--------|----------|-------------|
| GND | GND | Black |
| VCC | 5V | Red |
| SDA | A4 | Yellow |
| SCL | A5 | Green |

### Vibration Sensor (to Nano)

| Sensor | Nano Pin | Cable Color |
|--------|----------|-------------|
| GND | GND | Black |
| VCC | 5V | Red |
| OUT (DO) | A7 | Orange |

### Speaker (to Nano)

| Speaker | Nano Pin | Cable Color |
|---------|----------|-------------|
| (+) lead | D11 *(via 100Ω resistor)* | Blue |
| (−) lead | GND | Black |

**Important**: Wire the 100Ω resistor in series with the speaker's positive lead.
Without it, the 8Ω coil draws ~625 mA peak from the Nano pin (far above the 40 mA
maximum). The resistor limits current to ~31 mA and prevents damaging the pin or
the 0.25W speaker coil.

## Why These Pin Assignments?

The original design used a DS1302 RTC (SPI-like, ThreeWire library). The
HW-084 module uses a **DS1307** RTC which communicates over **I2C** on pins
**A4 (SDA)** and **A5 (SCL)**.

In the original wiring, the TFT's RES and CS pins were on A4 and A5 — which
would conflict with I2C. The fix:

- **TFT RES moved** from A4 → D5
- **TFT CS moved** from A5 → D2
- A4 and A5 are now free for the RTC's I2C bus

The TFT uses **software SPI** (bit-banged) on A1 (SCLK) and A2 (MOSI),
which works with any pins — no hardware SPI is required.

## Step-by-Step Assembly

### 1. Prepare the Nano

Place the Nano in the enclosure or on a breadboard. Identify the pin headers:

```
                         USB port (top)
  ┌──────────────────────────────────────────┐
  │ D12  D13  VIN  RST   A0  A1  A2  A3  A4 │
  │ D11  D10  D9   D8   D7  D6  D5  D4  D3  │
  │         GND  AREF  D2  D1  D0           │
  └──────────────────────────────────────────┘
                         USB port
```

### 2. Connect the TFT Display

1. **GND → GND** — any Nano GND pin (there are 3)
2. **VCC → 3.3V** — the ST7735 runs at 3.3V logic, do not use 5V here
3. **SCL → A1** — SPI clock (software bitbang)
4. **SDA → A2** — SPI data (MOSI)
5. **RES → D5** — Reset line
6. **DC → A3** — Data/Command select
7. **CS → D2** — Chip Select
8. **BL → A0** — Backlight control (code drives this HIGH on boot)

Keep wires short where possible to reduce signal noise on the display.

### 3. Connect the HW-084 RTC

1. Connect **GND → GND** (same ground rail as Nano)
2. Connect **VCC → 5V** (the HW-084 module uses 5V)
3. Connect **SDA → A4** (I2C data line)
4. Connect **SCL → A5** (I2C clock line)

Make sure the CR2032 battery is installed in the HW-084 holder. This keeps the
time when power is off.

**Note**: The HW-084 module commonly has pull-up resistors for the I2C bus, so
external pull-ups are not needed.

### 4. Connect the Vibration Sensor

1. **GND → GND**
2. **VCC → 5V** (most SW-420 modules work at 5V)
3. **DO (digital out) → A7**

Some vibration modules have an AO (analog out) pin. The code uses analog read
(A7) so either DO or AO works. If using DO, the threshold might need
adjustment — see the `THRESHOLD` define in the code.

### 5. Connect the Speaker

1. Solder or twist the **100Ω resistor** to the speaker's **positive** (usually
   red) wire
2. Connect the free end of the resistor → **D11**
3. Connect the speaker's **negative** (usually black) wire → **GND**

```
Nano D11 ─── 100Ω ─── Speaker(+) ─── Speaker(−) ─── GND
```

The 100Ω resistor is essential — without it the 8Ω coil would draw ~625 mA
peak from the Nano pin (ATmega328P max is 40 mA). The resistor limits current
to ~31 mA, which is safe for both the pin and the 0.25W speaker.

### 6. Connect Power

The Nano is powered via its micro-USB port. 5V and GND from the Nano's
regulator feed the RTC and vibration sensor. The TFT uses the 3.3V rail from
the Nano's onboard regulator.

### 7. Verify Continuity

Before plugging in USB, double-check with a multimeter:

- No shorts between VCC and GND on any module
- 3.3V pin measures ~3.3V relative to GND
- A4 and A5 are not connected to anything except the RTC (they should not
  touch TFT wires)

## Power Budget

| Component | Typical Current |
|-----------|----------------|
| Inland Nano (idle) | ~15 mA |
| ST7735 TFT (active) | ~30 mA |
| HW-084 RTC | ~1 mA |
| Vibration sensor | ~1 mA |
| 8Ω speaker (active) | ~5 mA (via 100Ω resistor) |
| **Total** | **~52 mA** |

Well within the Nano's USB power budget (500 mA). No external power supply
needed.

## Code Upload

1. Install the required libraries via Arduino Library Manager:
   - `Adafruit GFX Library` (by Adafruit)
   - `Adafruit ST7735 Library` (by Adafruit)
   - `RTClib` (by Adafruit)

2. Open `astrobot_hw084.ino` in the Arduino IDE

3. Select **Board: Arduino Nano**, **Processor: ATmega328P (Old Bootloader)**
   if using a clone, or **ATmega328P** for official Nanos. Inland Nanos
   typically need **Old Bootloader**.

4. Select the correct **Port**

5. Click **Upload**

## 3D Printed Enclosure Notes

When designing or printing the enclosure, consider:

- Leave ventilation gaps near the Nano's voltage regulator (it gets warm)
- The TFT should be recessed so the glass sits flush with the enclosure face
- The HW-084 needs access for the CR2032 battery — add a removable hatch or
  slot
- The vibration sensor should be mounted firmly so it picks up knocks on the
  enclosure wall
- Leave a hole for the micro-USB port
- Cable channels help keep wires tidy inside

## Troubleshooting

| Symptom | Likely Cause |
|---------|-------------|
| TFT blank/white | No power to TFT VCC; check 3.3V connection |
| TFT flickers | Loose CS or RES wire; check D2 and D5 |
| No time shown | HW-084 not connected; check A4/A5/SDA/SCL |
| Wrong time after power off | CR2032 battery missing or dead in HW-084 |
| Vibration not detected | Sensor GND not connected; threshold too high (`THRESHOLD`) |
| No sound from speaker | Resistor missing or speaker ± reversed; check D11 connection |
| Speaker sounds distorted/noisy | Missing 100Ω series resistor (overloading pin); or melody data issue |
| Won't upload | Wrong bootloader setting; try "Old Bootloader" |

## Code Reference

Key pin defines (from `astrobot_hw084.ino`):

```cpp
#define TFT_CS   2   // LCD chip select
#define TFT_RST  5   // LCD reset
#define TFT_DC   A3  // LCD data/command
#define TFT_MOSI A2  // LCD SPI data
#define TFT_SCLK A1  // LCD SPI clock
#define TFT_BL   A0  // LCD backlight

#define SPEAKER_PIN 11          // Melody output (via 100Ω resistor to speaker)

#define VIBRATION_SENSOR A7  // Vibration sensor analog input
#define THRESHOLD 500        // Vibration detection threshold

// RTC (DS1307) uses I2C: SDA=A4, SCL=A5
```

## Revision History

- **HW-084 revision** — Switched from DS1302 (ThreeWire) to DS1307 (I2C)
  via RTClib. TFT CS/RES moved off A4/A5 to free the I2C bus.
- **Modern TFT revision** — Replaced UTFT library with Adafruit_ST7735 +
  Adafruit_GFX for standard library compatibility.
