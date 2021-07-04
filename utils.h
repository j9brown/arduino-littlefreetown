#pragma once

#include <Print.h>
#include <TimeLib.h>

inline void assert(bool condition) {
  if (condition) return;

  while (true); // wait for watchdog reset
  __builtin_unreachable();
}

void printDateAndTime(Print& printer, time_t time);

struct RGB {
  uint8_t r, g, b;

  RGB operator*(float frac) const {
    return RGB{uint8_t(r * frac), uint8_t(g * frac), uint8_t(b * frac)};
  }
};

RGB colorWheel(uint8_t pos);
