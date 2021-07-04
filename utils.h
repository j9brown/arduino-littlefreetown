#pragma once

inline void assert(bool condition) {
  if (condition) return;

  while (true); // wait for watchdog reset
  __builtin_unreachable();
}

struct RGB {
  uint8_t r, g, b;

  RGB operator*(float frac) const {
    return RGB{uint8_t(r * frac), uint8_t(g * frac), uint8_t(b * frac)};
  }
};

inline RGB colorWheel(uint8_t pos) {
  pos = 255 - pos;
  if (pos < 85)
    return RGB{uint8_t(255 - pos * 3), 0, uint8_t(pos * 3)};
  if (pos < 170)
    return RGB{0, uint8_t((pos - 85) * 3), uint8_t(255 - (pos - 85) * 3)};
  return RGB{uint8_t((pos - 170) * 3), uint8_t(255 - (pos - 170) * 3), 0};
}
