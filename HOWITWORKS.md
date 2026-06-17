# Astrobot — How It Works

## What is Astrobot?

Astrobot is an animated display bot with expressive pixel-art eyes, a digital clock, vibration-reactive hearts, a speaker melody, and alarm clock features. It runs on an **Inland Nano** (an Arduino Nano clone) and uses a 1.8" TFT screen as its face. Most of the time it shows animated eyes that blink and look around. Every few seconds it switches to a digital clock display. If you knock on its enclosure, it plays a MIDI-converted melody while showing a beating heart animation. It can also wake you up at a configurable time with a looping alarm melody.

---

## Hardware at a glance

| Component | Role |
|-----------|------|
| **Inland Nano** | The brain. Runs the Arduino sketch, drives the display, reads the sensor, and plays sounds. |
| **HW-084 / DS1307 RTC** | The clock module. Keeps accurate time even when power is off thanks to a CR2032 coin cell battery. Communicates over I2C. |
| **1.8" ST7735 TFT** | The face. A 128x160 pixel color display that shows eyes, time, and hearts. |
| **SW-420 vibration sensor** | The ears. Detects knocks or taps on the enclosure so Astrobot can react. |
| **8 ohm 0.25W speaker + 100 ohm resistor** | The voice. Plays the melody when knocked or when the alarm goes off. The resistor limits current to protect the Nano's pin. |

**Wiring note**: The TFT uses software SPI on analog pins A1 (SCLK) and A2 (MOSI) instead of the hardware SPI pins. This frees up A4 and A5 for the RTC's I2C bus so both devices can work together without conflict.

---

## How the software works

The code runs a simple **state machine** that rotates through three states: eyes, clock, and vibration (hearts + melody). Each pass through `loop()` reads the vibration sensor, checks the RTC time, and decides which state to show.

### The state machine

- **Eyes state** — The default "idle" animation. Astrobot shows pixel-art eyes that blink and look left/right at random intervals (1--10 seconds between blinks). Each eye frame is drawn by plotting individual pixels from arrays stored in PROGMEM (flash memory) so they don't eat up the Nano's limited RAM. The eyes are mirrored across both sides of the screen.

- **Clock state** — After about 10 seconds of eyes, the code switches to a digital clock. The time is displayed in 12-hour format with a monospace font (Adafruit 5x7 at size 3), centered on the screen, amber-colored (255, 186, 31). After 5 seconds it flips back to eyes.

- **Hearts + Melody state (vibration)** — When the vibration sensor reading on A7 exceeds the threshold (default 500), the code jumps into this state. It clears the screen, starts playing the melody via `tone()`, and runs a heart-beating animation. It stays here until the melody finishes playing, then returns to eyes.

### Everything is non-blocking

The whole sketch avoids `delay()` entirely. Instead it uses `millis()` timers to track when things should happen. This means:

- The melody can keep playing while the hearts continue animating
- The state machine can switch states cleanly without freezing
- Multiple timers can run in parallel (animation frames, melody notes, state transitions)

The melody player is a mini state machine of its own. It tracks which note is playing, when that note started, and whether it's in the "sound" phase or the "silent gap" phase. Each `loop()` call to `updateMelody()` checks the elapsed time and advances to the next note or rest when the current phase is done.

---

## Features

### Clock
- 12-hour format with leading zeros (e.g., "09:05" not "9:5")
- AM/PM indicator
- Centered on the 128x160 screen using calculated offsets
- Amber color (`rgb(255, 186, 31)`) via `tft.color565()`
- Monospace font (Adafruit 5x7) at text size 3 for the digits, size 2 for the AM/PM label

### Eyes
- 16-frame animation sequence covering blink and look-left / look-right movements
- Frame 0 is the default "open" state — pauses for 1--10 seconds (random)
- Frames 1--4: eyes close downward (blink)
- Frames 5--8: eyes open back upward
- Frames 9--15: mirrored version of the blink sequence for looking right
- Each frame draws only the pixels that changed from the previous frame (uses separate "white" and "black" pixel arrays for additive/subtractive drawing)
- Pixel data stored in PROGMEM via `pgm_read_byte()`

### Vibration reaction
- Reads analog value on pin A7
- Threshold is configurable (default 500)
- When triggered: jumps to `VIBRATION_DETECTED` state
- Shows a 13+ frame beating heart animation (frames ramp up through heart sizes, then pulse)
- Starts the melody via `startMelody()`
- Returns to eyes when the melody finishes

### Melody
- Non-blocking player that converts MIDI note data into `tone()` calls
- Each note entry has three values: frequency (Hz), note duration (ms), and rest duration (ms)
- The player advances through notes by checking `millis()` against `melodyPhaseStart`
- Two-phase playback per entry: sound phase (tone on), then rest phase (silent gap)
- Plays during vibration state and alarm state
- Automatically stopped when exiting either state

### Alarm
- Configurable wake-up time (hour and minute)
- Weekday-only mode (skips alarm on weekends)
- When triggered: shows the time in red with an "ALARM" label
- Plays the melody on a loop until dismissed
- Dismissed by vibration (knock on the enclosure)
- Auto-off after 60 seconds if not dismissed

---

## Configuration reference

All of these are `#define` constants near the top of `astrobot_hw084.ino`:

| Constant | Values | Default | What it does |
|----------|--------|---------|-------------|
| `ALARM_ENABLED` | `true` / `false` | `true` | Enables or disables the alarm feature entirely |
| `ALARM_HOUR` | `0` -- `23` | `7` | The hour the alarm triggers (24-hour format) |
| `ALARM_MINUTE` | `0` -- `59` | `0` | The minute the alarm triggers |
| `WEEKDAYS_ONLY` | `true` / `false` | `true` | If `true`, alarm only fires Monday through Friday |
| `THRESHOLD` | `0` -- `1023` | `500` | Vibration sensitivity. Lower = more sensitive. |
| `SPEAKER_PIN` | Any digital pin | `11` | The pin connected to the speaker (via 100 ohm resistor) |
| `stateChangeIntervalTimer` | milliseconds | `5000` | How long the clock stays visible before switching back to eyes |
| `stateChangeIntervalEye` | milliseconds | `10000` | How long the eyes stay visible before switching to clock |

To change these, edit the `#define` lines and re-upload to the Nano.

---

## Pin reference

| Component | Signal | Nano Pin |
|-----------|--------|----------|
| **TFT** | CS | D2 |
| TFT | RST | D5 |
| TFT | DC | A3 |
| TFT | MOSI (data) | A2 |
| TFT | SCLK (clock) | A1 |
| TFT | BL (backlight) | A0 |
| **RTC (DS1307)** | SDA | A4 |
| RTC | SCL | A5 |
| **Vibration sensor** | OUT (analog) | A7 |
| **Speaker** | Positive (via 100 ohm) | D11 |
| Speaker | Negative | GND |

The TFT uses software SPI (bit-banged) on A1/A2. The RTC uses the hardware I2C bus on A4/A5. These don't interfere with each other.

---

## The non-blocking pattern

The entire sketch is built around `millis()` rather than `delay()`. Here's how each timer works:

```
loop():
  1. Call updateMelody()  -- advances melody if current note/rest is done
  2. Read vibration sensor
  3. Check state-change timers
  4. If state changed:
     - Stop any previous melody
     - Clear screen
     - Start new state (play melody, draw first eye frame, etc.)
  5. If animation interval has elapsed:
     - Advance to next frame (eyes or hearts)

  All of this happens in under a few milliseconds, then loop() runs again.
  The melody keeps playing while frames keep animating.
```

**Melody timing** (from `notes.h`):

```cpp
// Each entry: { frequency (Hz), note duration (ms), rest duration (ms) }
{ Ab3, 462, 0 },  // play Ab3 for 462ms, no rest afterward
{ Ab2, 115, 115 } // play Ab2 for 115ms, then rest 115ms
```

The `updateMelody()` function at line 268 of `astrobot_hw084.ino` tracks:
- `melodyIndex` — which note we're on
- `melodyPhaseStart` — when the current phase began
- `melodyPlaying` — whether we're in the tone phase or the rest phase

When a phase's duration elapses, the function either advances to the rest phase or moves to the next note. When all notes are done, `melodyActive` is set to `false` and playback stops.

This same pattern is used everywhere in the code: set a target time, check if it has passed, act if so. No blocking, no freezing, everything stays in sync.
