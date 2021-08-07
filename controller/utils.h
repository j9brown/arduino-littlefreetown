#pragma once

#include <Print.h>
#include <TimeLib.h>

inline void assert(bool condition) {
  if (condition) return;

  while (true); // wait for watchdog reset
  __builtin_unreachable();
}

void printDateAndTime(Print& printer, time_t time);

// Linear RGB color
struct RGBW;
struct RGB {
  uint8_t r, g, b;

  RGB operator*(float frac) const {
    return RGB{uint8_t(r * frac), uint8_t(g * frac), uint8_t(b * frac)};
  }

  bool operator==(const RGB& other) const {
    return r == other.r && g == other.g && b == other.b;
  }

  bool operator!=(const RGB& other) const { return !(*this == other); }

  inline RGBW toRGBW() const;
};

RGB colorWheel(uint8_t pos);

// Linear RGBW color
struct RGBW {
  uint8_t r, g, b, w;

  RGBW operator*(float frac) const {
    return RGBW{uint8_t(r * frac), uint8_t(g * frac), uint8_t(b * frac), uint8_t(w * frac)};
  }

  bool operator==(const RGBW& other) const {
    return r == other.r && g == other.g && b == other.b && w == other.w;
  }

  bool operator!=(const RGBW& other) const { return !(*this == other); }
};

inline RGBW RGB::toRGBW() const {
  return RGBW{r, g, b, 0};
}

// Lightness, Chroma, Hue representation
// More perceptually uniform than HSV though not all colors can be represented
// in RGB space.  See https://en.wikipedia.org/wiki/HCL_color_space.
struct LCH {
  float l; // lightness, 0 to 100+ (approx, depends on hue)
  float c; // chroma, 0 to 100+ (approx, depends on hue)
  float h; // hue, 0 to 360

  RGB toRGB() const;
};

// A color tint (hue) to apply to white light for illumination.
using tint_t = uint8_t;
constexpr tint_t TINT_WHITE = 0;
constexpr tint_t TINT_MIN = 0;
constexpr tint_t TINT_MAX = 36;
void printTint(Print& printer, tint_t tint);

// Brightness of a light source on a scale of 1 to 10.
using brightness_t = uint8_t;
constexpr brightness_t BRIGHTNESS_OFF = 0;
constexpr brightness_t BRIGHTNESS_MIN = 0;
constexpr brightness_t BRIGHTNESS_MAX = 10;
void printBrightness(Print& printer, brightness_t tint);

// Generates a color suitable for display on the panel knob.
RGB makeKnobColor(tint_t tint, brightness_t brightness);

// Generates a color suitable for display on an LED strip.
RGBW makeStripColor(tint_t tint, brightness_t brightness);

// Adds a multiple of a given step size to a value, clamps it to a range,
// if already at minimum or maximum, rolls over to the opposite end of the
// range.
template <typename T>
T addDeltaWithRollover(T value, T min, T max, T step, int32_t delta) {
  T newValue;
  if (delta > 0) {
    T adj = step * uint32_t(delta);
    if (value < max - adj) {
      newValue = value + adj;
    } else if (value != max) {
      newValue = max; // clamp
    } else {
      newValue = min; // roll-over
    }
  } else {
    T adj = step * uint32_t(-delta);
    if (value > min + adj) {
      newValue = value - adj;
    } else if (value != min) {
      newValue = min; // clamp
    } else {
      newValue = max; // roll-over
    }
  }
  return newValue;
}
