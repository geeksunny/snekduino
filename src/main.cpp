#include <Arduino.h>
#include "FastLED.h"

CRGB leds[NUM_LEDS];

CRGBPalette16 currentPalette;
TBlendType currentBlending;

void FillLEDsFromPaletteColors(uint8_t colorIndex) {
  for (auto & led : leds) {
    led = ColorFromPalette(currentPalette, colorIndex, BRIGHTNESS, currentBlending);
    colorIndex += 3;
  }
}

void setup() {
  delay(3000);  // power-up safety delay

  // Init LEDs
  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS);
  FastLED.setBrightness(BRIGHTNESS);

  // init palette and blending type
  currentPalette = RainbowColors_p;
  currentBlending = LINEARBLEND;

  Serial.println("setup() completed.");
}

void loop() {
  static uint8_t startIndex = 0;
  startIndex = startIndex + 1; /* motion speed */

  FillLEDsFromPaletteColors( startIndex);

  FastLED.show();
  FastLED.delay(1000 / REFRESH_RATE);
}
