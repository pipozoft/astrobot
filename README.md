# Astrobot

An expressive eye/display bot for the **Inland Nano** (Arduino Nano clone, ATmega328P) with a **1.8" ST7735 TFT**, **HW-084 DS1307 RTC**, and **vibration sensor**.

Cycles between animated pixel-art eyes, a digital clock, and a heartbeat animation on vibration.

## Features

- **Animated eyes** — 5-frame blink/wink sequence with smooth transitions
- **Digital clock** — HH:MM readout with colon blinking, synced from the DS1307 RTC
- **Vibration reaction** — SW-420 triggers a 13-frame heartbeat animation
- **Pixel-art graphics** — All sprites stored in PROGMEM via an external header

## Hardware

| Component | Qty |
|-----------|-----|
| Inland Nano (Arduino Nano clone) | 1 |
| HW-084 (DS1307 RTC with coin cell) | 1 |
| 1.8" 128×160 TFT (ST7735, 8-pin) | 1 |
| SW-420 vibration sensor | 1 |

See [docs/WIRING_GUIDE.md](docs/WIRING_GUIDE.md) for the full wiring table, pin-reassignment rationale, and enclosure notes.

## Dependencies

Install these via the Arduino Library Manager:

- [Adafruit GFX](https://github.com/adafruit/Adafruit-GFX-Library)
- [Adafruit ST7735](https://github.com/adafruit/Adafruit-ST7735-Library)
- [RTClib](https://github.com/adafruit/RTClib)

## Upload

1. Open `astrobot_hw084.ino` in the Arduino IDE.
2. Select **Tools → Board → Arduino Nano** (or Inland Nano).
3. Select the correct **Processor** (ATmega328P, Old Bootloader if needed).
4. Select the port and upload.

## Project Structure

```
astrobot_hw084/
├── astrobot_hw084.ino      # Main sketch
├── pixel_data.h            # All 23 eye/heart frames as PROGMEM arrays
├── notes.h                 # Pin mappings, constants, and notes
├── HOWITWORKS.md           # Under-the-hood explanation
├── README.md
├── .gitignore
├── 3d/
│   └── astrobot-final-version.3mf   # 3D-printable enclosure
├── docs/
│   └── WIRING_GUIDE.md     # Step-by-step build instructions
├── eyes/
│   ├── ImagesToCArray.py   # PNG → PROGMEM C array converter
│   ├── normal_eye/         # 5 normal eye animation frames (PNG)
│   │   ├── astrobot1-5.png
│   │   └── output.h
│   ├── heart_eye/          # 13 heart eye animation frames (PNG)
│   │   ├── astrobot11-23.png
│   │   └── output.h
│   └── .gitignore
└── music/
    ├── Light MIDI - Astro Bot Theme.mid
    └── Light MIDI - Astro Bot Theme.mp3
```


## Credit

Project based on [Astrobot Clock](https://github.com/Birdy-C/Puzzle-with-3D-Printer/tree/main/Astrobot%20Clock)