#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <RTClib.h>
#include "pixel_data.h"

enum State {
  SHOW_EYES,
  SHOW_DATE,
  VIBRATION_DETECTED
};

#define VIBRATION_SENSOR A7
#define THRESHOLD 500

#define TFT_CS   2
#define TFT_RST  5
#define TFT_DC   A3
#define TFT_MOSI A2
#define TFT_SCLK A1
#define TFT_BL   A0

RTC_DS1307 rtc;
Adafruit_ST7735 tft(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCLK, TFT_RST);

#define countof(a) (sizeof(a) / sizeof(a[0]))

void printDateTime(const DateTime& dt) {
  char buf[20];
  snprintf_P(buf, countof(buf), PSTR("%02u/%02u/%04u %02u:%02u:%02u"),
    dt.month(), dt.day(), dt.year(), dt.hour(), dt.minute(), dt.second());
  Serial.print(buf);
}

void setupRtc() {
  Serial.print("compiled: ");
  Serial.print(__DATE__);
  Serial.println(__TIME__);

  if (!rtc.begin()) {
    Serial.println("Couldn't find RTC (DS1307) on I2C bus!");
    Serial.println("Check wiring: SDA -> A4, SCL -> A5");
    while (1) delay(10);
  }

  DateTime compiled = DateTime(__DATE__, __TIME__);
  printDateTime(compiled);
  Serial.println();

  if (!rtc.isrunning()) {
    Serial.println("RTC is NOT running — setting to compile time.");
    rtc.adjust(compiled);
  } else {
    DateTime now = rtc.now();
    if (now < compiled) {
      Serial.println("RTC time is before compile time — updating.");
      rtc.adjust(compiled);
    }
  }
}

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

void process_pixel_array_double(unsigned char pixels[][2], int size) {
  for (int i = 0; i < size; i++) {
    int x = pgm_read_byte(&pixels[i][0]);
    int y = pgm_read_byte(&pixels[i][1]);
    tft.drawPixel(x, y, currentColor);
    tft.drawPixel(x + 80, y, currentColor);
  }
}

void process_pixel_array_double_invert(unsigned char pixels[][2], int size) {
  for (int i = 0; i < size; i++) {
    int x = 77 - pgm_read_byte(&pixels[i][0]);
    int y = pgm_read_byte(&pixels[i][1]);
    tft.drawPixel(x, y, currentColor);
    tft.drawPixel(x + 80, y, currentColor);
  }
}

unsigned long animation_interval = 200;

void setBlackColor()   { setColor(ST77XX_BLACK); }
void setEyeColor()     { setColor(tft.color565(255, 186, 31)); }
void setHeartColor()   { setColor(tft.color565(0, 0, 255)); }

void printDate(const DateTime& dt) {
  uint8_t hour12 = dt.hour() % 12;
  if (hour12 == 0) hour12 = 12;
  const char* ampm = dt.hour() < 12 ? "AM" : "PM";

  char timeStr[6];
  snprintf(timeStr, sizeof(timeStr), "%02u:%02u", hour12, dt.minute());

  uint16_t color = tft.color565(255, 186, 31);

  int16_t tw = 5 * 6 * 3;
  int16_t aw = 3 * 6 * 2;
  int16_t x0 = (160 - (tw + aw)) / 2;

  tft.setTextColor(color, ST77XX_BLACK);

  tft.setTextSize(3);
  tft.setCursor(x0, 40);
  tft.print(timeStr);

  tft.setTextSize(2);
  tft.setCursor(x0 + tw, 47);
  tft.print(" ");
  tft.print(ampm);
}

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

State previousState = SHOW_DATE;
State currentState = SHOW_DATE;

unsigned long vibrationDetectedMillis = 0;
unsigned long stateChangeMillis = 0;
const unsigned long stateChangeIntervalTimer = 5000;
const unsigned long stateChangeIntervalEye = 10000;
const unsigned long stateChangeIntervalVib = 5000;

unsigned long animation_started = 0;
int animation_loop = 0;

void loop() {
  DateTime now = rtc.now();
  int vibrationValue = analogRead(VIBRATION_SENSOR);
  unsigned long currentMillis = millis();

  previousState = currentState;

  if (vibrationValue > THRESHOLD) {
    Serial.println("vibration detected");
    currentState = VIBRATION_DETECTED;
    vibrationDetectedMillis = currentMillis;
  } else {
    if (previousState == SHOW_EYES && currentMillis - stateChangeMillis >= stateChangeIntervalEye) {
      currentState = SHOW_DATE;
    } else if (previousState == SHOW_DATE && currentMillis - stateChangeMillis >= stateChangeIntervalTimer) {
      currentState = SHOW_EYES;
    } else if (previousState == VIBRATION_DETECTED && currentMillis - vibrationDetectedMillis >= stateChangeIntervalVib) {
      currentState = SHOW_EYES;
    }
  }

  if (currentState != previousState) {
    stateChangeMillis = currentMillis;
    animation_started = currentMillis;
    animation_loop = 0;
    clearScreen();

    switch (currentState) {
      case VIBRATION_DETECTED:
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
