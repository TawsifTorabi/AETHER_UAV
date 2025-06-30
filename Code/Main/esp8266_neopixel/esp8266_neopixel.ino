#include <Adafruit_NeoPixel.h>

#define LED_PIN    D5          // NeoPixel data pin
#define LED_COUNT  24
#define BRIGHTNESS 255

#define CONTROL_PIN D6         // GPIO12 — control input (active when NOT grounded)

Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

// Tail fade intensity (lower = longer tail)
const float fadeFactor = 0.6;
const int delayTime = 40; // ms between frames

void setup() {
  pinMode(CONTROL_PIN, INPUT_PULLUP); // HIGH by default unless pulled LOW
  strip.begin();
  strip.setBrightness(BRIGHTNESS);
  strip.show(); // Start off
}

void loop() {
  if (digitalRead(CONTROL_PIN) == HIGH) {
    // D6 NOT grounded → run beacon
    sweepColor(strip.Color(255, 0, 0), 0, LED_COUNT, 1);              // Red sweep → right
    delay(100);
    sweepColor(strip.Color(0, 0, 255), LED_COUNT - 1, -1, -1);        // Blue sweep ← left
    delay(100);
  } else {
    // D6 is grounded → turn LEDs off
    strip.clear();
    strip.show();
  }
}

void sweepColor(uint32_t color, int start, int end, int step) {
  for (int i = start; i != end; i += step) {
    fadeAll();

    strip.setPixelColor(i, color);  // Leading light
    strip.show();
    delay(delayTime);
  }
}

// Fades all pixels by a factor
void fadeAll() {
  for (int i = 0; i < LED_COUNT; i++) {
    uint32_t c = strip.getPixelColor(i);
    byte r = (uint8_t)(c >> 16);
    byte g = (uint8_t)(c >> 8);
    byte b = (uint8_t)(c);

    r *= fadeFactor;
    g *= fadeFactor;
    b *= fadeFactor;

    strip.setPixelColor(i, r, g, b);
  }
}
