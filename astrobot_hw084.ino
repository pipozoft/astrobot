// Astrobot — animated display bot
// Eyes, clock, vibration-reactive hearts, speaker melody, and alarm clock.
// State machine cycles between SHOW_EYES, SHOW_DATE, VIBRATION_DETECTED, ALARMING.

#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <RTClib.h>
#include "pixel_data.h"
#include "notes.h"

// ============================================================
// Configuration — adjust these to change behavior
// ============================================================

// Alarm clock
#define ALARM_ENABLED   true
#define ALARM_HOUR      20
#define ALARM_MINUTE    16
#define WEEKDAYS_ONLY   true

// Vibration sensor
#define VIBRATION_SENSOR A7
#define THRESHOLD 500

// ============================================================
// Pin assignments
// ============================================================

// TFT display (1.8" ST7735, software SPI)
#define TFT_CS   2
#define TFT_RST  5
#define TFT_DC   A3
#define TFT_MOSI A2
#define TFT_SCLK A1
#define TFT_BL   A0

// Speaker (8 ohm 0.25W via 100 ohm resistor)
#define SPEAKER_PIN 11

// RTC (DS1307) uses I2C: SDA=A4, SCL=A5

RTC_DS1307 rtc;
Adafruit_ST7735 tft(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCLK, TFT_RST);

#define countof(a) (sizeof(a) / sizeof(a[0]))

// ============================================================
// State machine — each state shows a different "mood"
// ============================================================

enum State {
  SHOW_EYES,          // Animated pixel-art eyes (default state)
  SHOW_DATE,          // Digital 12h clock, amber text
  VIBRATION_DETECTED, // Heart animation + melody when knocked
  ALARMING            // Alarm triggered — red time + looping melody
};

// ============================================================
// RTC setup — syncs time on first boot
// ============================================================

void setupRtc() {
  Serial.print("compiled: ");
  Serial.print(__DATE__);
  Serial.println(__TIME__);

  if (!rtc.begin()) {
    Serial.println("RTC not found! Check A4/A5");
    while (1) delay(10);
  }

  if (!rtc.isrunning()) {
    Serial.println("RTC stale! Setting from compile time");
    rtc.adjust(DateTime(__DATE__, __TIME__));
  } else {
    DateTime now = rtc.now();
    DateTime compiled = DateTime(__DATE__, __TIME__);
    if (now < compiled) {
      Serial.println("RTC behind — updated to computer time");
      rtc.adjust(compiled);
    }
  }
}

// ============================================================
// TFT setup — initializes the display
// ============================================================

void setupLCD() {
  pinMode(TFT_BL, OUTPUT);
  digitalWrite(TFT_BL, HIGH);
  tft.initR(INITR_BLACKTAB);
  tft.setRotation(1);
  tft.fillScreen(ST77XX_BLACK);
}

void setup() {
  Serial.begin(9600);
  randomSeed(analogRead(0));
  setupRtc();
  setupLCD();
}

// ============================================================
// Pixel drawing helpers — render eye/heart frames from PROGMEM
// ============================================================

#define PROCESS_PIXEL_ARRAY(array) process_pixel_array(array, sizeof(array) / sizeof(array[0]))
#define PROCESS_PIXEL_ARRAY_DOUBLE(array) process_pixel_array_double(array, sizeof(array) / sizeof(array[0]))
#define PROCESS_PIXEL_ARRAY_DOUBLE_INVERT(array) process_pixel_array_double_invert(array, sizeof(array) / sizeof(array[0]))

uint16_t currentColor;

void setColor(uint16_t c) { currentColor = c; }

void process_pixel_array(unsigned char pixels[][2], int size) {
  for (int i = 0; i < size; i++) {
    int x = pgm_read_byte(&pixels[i][0]);
    int y = pgm_read_byte(&pixels[i][1]);
    tft.drawPixel(x, y, currentColor);
  }
}

// Draw the same frame on both eyes (left eye, right eye shifted +80)
void process_pixel_array_double(unsigned char pixels[][2], int size) {
  for (int i = 0; i < size; i++) {
    int x = pgm_read_byte(&pixels[i][0]);
    int y = pgm_read_byte(&pixels[i][1]);
    tft.drawPixel(x, y, currentColor);
    tft.drawPixel(x + 80, y, currentColor);
  }
}

// Same but left eye is mirrored horizontally
void process_pixel_array_double_invert(unsigned char pixels[][2], int size) {
  for (int i = 0; i < size; i++) {
    int x = 77 - pgm_read_byte(&pixels[i][0]);
    int y = pgm_read_byte(&pixels[i][1]);
    tft.drawPixel(x, y, currentColor);
    tft.drawPixel(x + 80, y, currentColor);
  }
}

// ============================================================
// Color helpers — eye amber, heart blue, black (erase)
// ============================================================

unsigned long animation_interval = 200;

void setBlackColor()   { setColor(ST77XX_BLACK); }
void setEyeColor()     { setColor(tft.color565(255, 255, 255)); }
void setHeartColor()   { setColor(tft.color565(255, 0, 0)); }

// ============================================================
// Clock display — 12h format, monospace (Adafruit 5x7), centered
// ============================================================

void printDate(const DateTime& dt) {
  uint8_t h12 = dt.hour() % 12;
  if (h12 == 0) h12 = 12;
  char t[6];
  t[0] = '0' + h12 / 10;
  t[1] = '0' + h12 % 10;
  t[2] = ':';
  t[3] = '0' + dt.minute() / 10;
  t[4] = '0' + dt.minute() % 10;
  t[5] = '\0';

  int16_t tw = 5 * 6 * 3;
  int16_t aw = 3 * 6 * 2;
  int16_t x0 = (160 - (tw + aw)) / 2;

  tft.setTextColor(tft.color565(255, 186, 31), ST77XX_BLACK);
  tft.setTextSize(3);
  tft.setCursor(x0, 40);
  tft.print(t);
  tft.setTextSize(2);
  tft.setCursor(x0 + tw, 47);
  tft.print(" ");
  tft.print(dt.hour() < 12 ? "AM" : "PM");
}

#if ALARM_ENABLED
void printAlarm(const DateTime& dt) {
  uint8_t h12 = dt.hour() % 12;
  if (h12 == 0) h12 = 12;
  char t[6];
  t[0] = '0' + h12 / 10;
  t[1] = '0' + h12 % 10;
  t[2] = ':';
  t[3] = '0' + dt.minute() / 10;
  t[4] = '0' + dt.minute() % 10;
  t[5] = '\0';

  int16_t tw = 5 * 6 * 3;
  int16_t aw = 3 * 6 * 2;
  int16_t x0 = (160 - (tw + aw)) / 2;

  tft.setTextColor(tft.color565(255, 0, 0), ST77XX_BLACK);
  tft.setTextSize(3);
  tft.setCursor(x0, 30);
  tft.print(t);
  tft.setTextSize(2);
  tft.setCursor(x0 + tw, 37);
  tft.print(" ");
  tft.print(dt.hour() < 12 ? "AM" : "PM");

  tft.setTextSize(2);
  tft.setTextColor(tft.color565(255, 0, 0), ST77XX_BLACK);
  tft.setCursor((160 - 5 * 6 * 2) / 2, 70);
  tft.print("ALARM");
}
#endif

// ============================================================
// Eye animation — blink/look sequences
// ============================================================

int drawEyes(int frame) {
  animation_interval = 100;
  int step = frame % 16;

  if (frame == 0) {
    setEyeColor();
    PROCESS_PIXEL_ARRAY_DOUBLE(white_pixels_astrobot1);
    animation_interval = random(1000, 10000);
    return random(2) * 6;
  }
  if (step == 1) {
    setEyeColor(); PROCESS_PIXEL_ARRAY_DOUBLE(white_pixels_astrobot2);
    setBlackColor(); PROCESS_PIXEL_ARRAY_DOUBLE(black_pixels_astrobot2);
  } else if (step == 2) {
    setEyeColor(); PROCESS_PIXEL_ARRAY_DOUBLE(white_pixels_astrobot3);
    setBlackColor(); PROCESS_PIXEL_ARRAY_DOUBLE(black_pixels_astrobot3);
  } else if (step == 3) {
    setEyeColor(); PROCESS_PIXEL_ARRAY_DOUBLE(white_pixels_astrobot4);
    setBlackColor(); PROCESS_PIXEL_ARRAY_DOUBLE(black_pixels_astrobot4);
  } else if (step == 4) {
    setEyeColor(); PROCESS_PIXEL_ARRAY_DOUBLE(white_pixels_astrobot5);
    setBlackColor(); PROCESS_PIXEL_ARRAY_DOUBLE(black_pixels_astrobot5);
    animation_interval = random(1000, 4000);
  } else if (step == 5) {
    setEyeColor(); PROCESS_PIXEL_ARRAY_DOUBLE(black_pixels_astrobot5);
    setBlackColor(); PROCESS_PIXEL_ARRAY_DOUBLE(white_pixels_astrobot5);
  } else if (step == 6) {
    setEyeColor(); PROCESS_PIXEL_ARRAY_DOUBLE(black_pixels_astrobot4);
    setBlackColor(); PROCESS_PIXEL_ARRAY_DOUBLE(white_pixels_astrobot4);
  } else if (step == 7) {
    setEyeColor(); PROCESS_PIXEL_ARRAY_DOUBLE(black_pixels_astrobot3);
    setBlackColor(); PROCESS_PIXEL_ARRAY_DOUBLE(white_pixels_astrobot3);
  } else if (step == 8) {
    setEyeColor(); PROCESS_PIXEL_ARRAY_DOUBLE(black_pixels_astrobot2);
    setBlackColor(); PROCESS_PIXEL_ARRAY_DOUBLE(white_pixels_astrobot2);
    animation_interval = random(1000, 4000);
  } else if (step == 9) {
    setEyeColor(); PROCESS_PIXEL_ARRAY_DOUBLE_INVERT(white_pixels_astrobot2);
    setBlackColor(); PROCESS_PIXEL_ARRAY_DOUBLE_INVERT(black_pixels_astrobot2);
  } else if (step == 10) {
    setEyeColor(); PROCESS_PIXEL_ARRAY_DOUBLE_INVERT(white_pixels_astrobot3);
    setBlackColor(); PROCESS_PIXEL_ARRAY_DOUBLE_INVERT(black_pixels_astrobot3);
  } else if (step == 11) {
    setEyeColor(); PROCESS_PIXEL_ARRAY_DOUBLE_INVERT(white_pixels_astrobot4);
    setBlackColor(); PROCESS_PIXEL_ARRAY_DOUBLE_INVERT(black_pixels_astrobot4);
  } else if (step == 12) {
    setEyeColor(); PROCESS_PIXEL_ARRAY_DOUBLE_INVERT(white_pixels_astrobot5);
    setBlackColor(); PROCESS_PIXEL_ARRAY_DOUBLE_INVERT(black_pixels_astrobot5);
    animation_interval = random(1000, 4000);
  } else if (step == 13) {
    setEyeColor(); PROCESS_PIXEL_ARRAY_DOUBLE_INVERT(black_pixels_astrobot5);
    setBlackColor(); PROCESS_PIXEL_ARRAY_DOUBLE_INVERT(white_pixels_astrobot5);
  } else if (step == 14) {
    setEyeColor(); PROCESS_PIXEL_ARRAY_DOUBLE_INVERT(black_pixels_astrobot4);
    setBlackColor(); PROCESS_PIXEL_ARRAY_DOUBLE_INVERT(white_pixels_astrobot4);
  } else if (step == 15) {
    setEyeColor(); PROCESS_PIXEL_ARRAY_DOUBLE_INVERT(black_pixels_astrobot3);
    setBlackColor(); PROCESS_PIXEL_ARRAY_DOUBLE_INVERT(white_pixels_astrobot3);
  }
  return frame;
}

// ============================================================
// Heart animation — beating heart on vibration
// ============================================================

int drawHearts(int frame) {
  animation_interval = 200;
  if (frame == 0) {
    setHeartColor(); PROCESS_PIXEL_ARRAY_DOUBLE(white_pixels_astrobot11);
  } else if (frame == 1) {
    setHeartColor(); PROCESS_PIXEL_ARRAY_DOUBLE(white_pixels_astrobot12);
  } else if (frame == 2) {
    setHeartColor(); PROCESS_PIXEL_ARRAY_DOUBLE(white_pixels_astrobot13);
  } else if (frame == 3) {
    setHeartColor(); PROCESS_PIXEL_ARRAY_DOUBLE(white_pixels_astrobot14);
  } else if (frame == 4) {
    setHeartColor(); PROCESS_PIXEL_ARRAY_DOUBLE(white_pixels_astrobot15);
  } else if (frame == 5) {
    setHeartColor(); PROCESS_PIXEL_ARRAY_DOUBLE(white_pixels_astrobot16);
  } else if (frame == 6) {
    setHeartColor(); PROCESS_PIXEL_ARRAY_DOUBLE(white_pixels_astrobot17);
  } else if (frame == 7) {
    setHeartColor(); PROCESS_PIXEL_ARRAY_DOUBLE(white_pixels_astrobot18);
  } else if (frame == 8) {
    setHeartColor(); PROCESS_PIXEL_ARRAY_DOUBLE(white_pixels_astrobot19);
  } else {
    int s = frame % 8;
    if (s == 1) {
      setHeartColor(); PROCESS_PIXEL_ARRAY_DOUBLE(white_pixels_astrobot20);
    } else if (s == 2) {
      setHeartColor(); PROCESS_PIXEL_ARRAY_DOUBLE(white_pixels_astrobot21);
    } else if (s == 3) {
      setHeartColor(); PROCESS_PIXEL_ARRAY_DOUBLE(white_pixels_astrobot22);
    } else if (s == 4) {
      setHeartColor(); PROCESS_PIXEL_ARRAY_DOUBLE(white_pixels_astrobot23);
    } else if (s == 5) {
      setBlackColor(); PROCESS_PIXEL_ARRAY_DOUBLE(white_pixels_astrobot23);
    } else if (s == 6) {
      setBlackColor(); PROCESS_PIXEL_ARRAY_DOUBLE(white_pixels_astrobot22);
    } else if (s == 7) {
      setBlackColor(); PROCESS_PIXEL_ARRAY_DOUBLE(white_pixels_astrobot21);
    } else {
      setBlackColor(); PROCESS_PIXEL_ARRAY(white_pixels_astrobot20);
      animation_interval = 500;
    }
  }
  return frame;
}

void clearScreen() {
  tft.fillScreen(ST77XX_BLACK);
}

// ============================================================
// Melody player — non-blocking, driven by millis()
// ============================================================

int melodyPin;
const int (*melodyNotes)[3];
size_t melodyLen;
int melodyIndex;
unsigned long melodyPhaseStart;
bool melodyActive;
bool melodyPlaying;

void startMelody(int pin, const int notes[][3], size_t len) {
  melodyPin = pin;
  melodyNotes = notes;
  melodyLen = len;
  melodyIndex = 0;
  melodyActive = true;
  melodyPlaying = true;
  melodyPhaseStart = millis();
  tone(melodyPin, melodyNotes[0][0]);
}

void updateMelody() {
  if (!melodyActive || melodyIndex >= melodyLen) return;

  unsigned long now = millis();
  unsigned long elapsed = now - melodyPhaseStart;

  if (melodyPlaying) {
    if (elapsed >= melodyNotes[melodyIndex][1]) {
      noTone(melodyPin);
      if (melodyNotes[melodyIndex][2] > 0) {
        melodyPlaying = false;
        melodyPhaseStart = now;
      } else {
        melodyIndex++;
        if (melodyIndex >= melodyLen) {
          melodyActive = false;
        } else {
          melodyPhaseStart = now;
          tone(melodyPin, melodyNotes[melodyIndex][0]);
        }
      }
    }
  } else {
    if (elapsed >= melodyNotes[melodyIndex][2]) {
      melodyIndex++;
      if (melodyIndex >= melodyLen) {
        melodyActive = false;
      } else {
        melodyPlaying = true;
        melodyPhaseStart = now;
        tone(melodyPin, melodyNotes[melodyIndex][0]);
      }
    }
  }
}

bool isMelodyDone() {
  return !melodyActive;
}

// ============================================================
// State machine — manages eyes/clock/hearts/alarm
// ============================================================

State previousState = SHOW_DATE;
State currentState = SHOW_DATE;

unsigned long stateChangeMillis = 0;
const unsigned long stateChangeIntervalTimer = 5000;
const unsigned long stateChangeIntervalEye = 10000;

unsigned long animation_started = 0;
int animation_loop = 0;

// Alarm state — trigger, dismiss, auto-off
#if ALARM_ENABLED
bool alarmDismissedToday = false;
#endif

// ============================================================
// Main loop
// ============================================================

void loop() {
  // 1. Update melody (runs every tick, non-blocking)
  updateMelody();

  // Restart melody when it finishes during alarm (looping)
  #if ALARM_ENABLED
  if (currentState == ALARMING && isMelodyDone()) {
    startMelody(SPEAKER_PIN, midi1, countof(midi1));
  }
  #endif

  // 2. Read time and vibration sensor
  DateTime now = rtc.now();
  int vibrationValue = analogRead(VIBRATION_SENSOR);
  unsigned long currentMillis = millis();

  previousState = currentState;

  // 3. State transitions (priority order)
  if (vibrationValue > THRESHOLD) {
    Serial.println("vibration detected");
    #if ALARM_ENABLED
    if (currentState == ALARMING) {
      currentState = SHOW_EYES;
      alarmDismissedToday = true;
    } else
    #endif
    {
      currentState = VIBRATION_DETECTED;
    }
  } else {
    #if ALARM_ENABLED
    // Check alarm trigger
    if (currentState != ALARMING && !alarmDismissedToday &&
        now.hour() == ALARM_HOUR && now.minute() == ALARM_MINUTE &&
        now.second() < 5) {
      if (!WEEKDAYS_ONLY || (now.dayOfTheWeek() >= 1 && now.dayOfTheWeek() <= 5)) {
        currentState = ALARMING;
      }
    }

    // Normal transitions (skip when alarming)
    if (currentState != ALARMING) {
    #endif
      if (previousState == SHOW_EYES && currentMillis - stateChangeMillis >= stateChangeIntervalEye) {
        currentState = SHOW_DATE;
      } else if (previousState == SHOW_DATE && currentMillis - stateChangeMillis >= stateChangeIntervalTimer) {
        currentState = SHOW_EYES;
      } else if (previousState == VIBRATION_DETECTED && isMelodyDone()) {
        currentState = SHOW_EYES;
      }
    #if ALARM_ENABLED
    }

    // Auto-dismiss alarm after 60s
    if (currentState == ALARMING && currentMillis - stateChangeMillis >= 60000) {
      currentState = SHOW_EYES;
      alarmDismissedToday = true;
    }

    // Reset dismiss flag at midnight
    if (now.hour() == 0 && now.minute() == 0 && now.second() < 5) {
      alarmDismissedToday = false;
    }
    #endif
  }

  // 4. State entry — runs once when switching to a new state
  if (currentState != previousState) {
    if (previousState == VIBRATION_DETECTED || previousState == ALARMING) {
      noTone(melodyPin);
      melodyActive = false;
      melodyIndex = 0;
    }
    stateChangeMillis = currentMillis;
    animation_started = currentMillis;
    animation_loop = 0;
    clearScreen();

    switch (currentState) {
      #if ALARM_ENABLED
      case ALARMING:
        startMelody(SPEAKER_PIN, midi1, countof(midi1));
        printAlarm(now);
        break;
      #endif
      case VIBRATION_DETECTED:
        startMelody(SPEAKER_PIN, midi1, countof(midi1));
        animation_loop = drawHearts(animation_loop);
        break;
      case SHOW_EYES:
        animation_loop = drawEyes(animation_loop);
        break;
      case SHOW_DATE:
        printDate(now);
        break;
      default: break;
    }
  }

  // 5. Animation ticks — runs periodically based on animation_interval
  if (currentMillis - animation_started > animation_interval) {
    animation_started = currentMillis;
    animation_loop++;

    switch (currentState) {
      case VIBRATION_DETECTED:
        animation_loop = drawHearts(animation_loop);
        break;
      case SHOW_EYES:
        animation_loop = drawEyes(animation_loop);
        break;
      default: break;
    }
  }
}
