#include <Arduino.h>
#include "FastLED.h"
#include "qmk.h"

extern "C" {
#include "snek.inc"
}

#define CHAR_ESCAPE_PREFIX    27
#define CHAR_ESCAPE_SEQUENCE  91
#define CHAR_ARROW_UP         65
#define CHAR_ARROW_DOWN       66
#define CHAR_ARROW_LEFT       68
#define CHAR_ARROW_RIGHT      67

enum class KeyboardInput {
  UP, DOWN, LEFT, RIGHT, INVALID, NONE
};

char input;
KeyboardInput inputKey = KeyboardInput::NONE;

CRGB leds[NUM_LEDS];

CRGBPalette16 currentPalette;
TBlendType currentBlending;

void FillLEDsFromPaletteColors(uint8_t colorIndex) {
  for (auto & led : leds) {
    led = ColorFromPalette(currentPalette, colorIndex, BRIGHTNESS, currentBlending);
    colorIndex += 3;
  }
}

KeyboardInput readInput() {
  if (Serial.available()) {
    input = Serial.read();
    if (input == CHAR_ESCAPE_PREFIX) {
      bool awaiting_sequence_char = true;
      while (true) {
        while (!Serial.available()) { /* Wait */ }
        input = Serial.read();
        if (awaiting_sequence_char) {
          if (input == CHAR_ESCAPE_SEQUENCE) {
            awaiting_sequence_char = false;
          } else {
            return KeyboardInput::INVALID;
          }
        } else {
          switch (input) {
            case CHAR_ARROW_UP:
              return KeyboardInput::UP;
            case CHAR_ARROW_DOWN:
              return KeyboardInput::DOWN;
            case CHAR_ARROW_LEFT:
              return KeyboardInput::LEFT;
            case CHAR_ARROW_RIGHT:
              return KeyboardInput::RIGHT;
            default:
              return KeyboardInput::INVALID;
          }
        }
      }
    } else {
      return KeyboardInput::INVALID;
    }
  } else {
    return KeyboardInput::NONE;
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

  // Init Snake Game
  snake_init();

  Serial.println("setup() completed.");
}

void loop() {
  static uint8_t startIndex = 0;
  startIndex = startIndex + 1; /* motion speed */

  FillLEDsFromPaletteColors( startIndex);

  FastLED.show();
  FastLED.delay(1000 / REFRESH_RATE);

  snake_exec();

  inputKey = readInput();
  switch (inputKey) {
    case KeyboardInput::UP:
      snake_set_next_direction(UP);
      break;
    case KeyboardInput::DOWN:
      snake_set_next_direction(DOWN);
      break;
    case KeyboardInput::LEFT:
      snake_set_next_direction(LEFT);
      break;
    case KeyboardInput::RIGHT:
      snake_set_next_direction(RIGHT);
      break;
    case KeyboardInput::INVALID:
      break;
    case KeyboardInput::NONE:
      break;
  }
}
