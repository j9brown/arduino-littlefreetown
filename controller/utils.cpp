#include <math.h>

#include <Arduino.h>

#include "utils.h"

#define USE_LCH_COLOR 0

namespace {
constexpr float M_PI_180 = M_PI / 180;
} // namespace

void printDateAndTime(Print& printer, time_t time) {
  TimeElements te;
  breakTime(time, te);
  printer.print(tmYearToCalendar(te.Year));
  printer.print('/');
  printer.print(te.Month);
  printer.print('/');
  printer.print(te.Day);
  printer.print(' ');
  printer.print(te.Hour);
  printer.print(':');
  if (te.Minute < 10) printer.print('0');
  printer.print(te.Minute);
  printer.print(':');
  if (te.Second < 10) printer.print('0');
  printer.print(te.Second);
}

uint8_t scaleAndClampRgb(float t, float scale) {
  t *= scale;
  if (t <= 0.1f) return 0;
  if (t >= 255.f) return 255;
  if (t <= 1.f) return 1;
  return uint8_t(roundf(t));
}

RGB RGB::colorWheel(uint8_t pos) {
  pos = 255 - pos;
  if (pos < 85)
    return RGB{uint8_t(255 - pos * 3), 0, uint8_t(pos * 3)};
  if (pos < 170)
    return RGB{0, uint8_t((pos - 85) * 3), uint8_t(255 - (pos - 85) * 3)};
  return RGB{uint8_t((pos - 170) * 3), uint8_t(255 - (pos - 170) * 3), 0};
}

RGB RGB::operator*(float scale) const {
  return RGB{
    scaleAndClampRgb(r, scale),
    scaleAndClampRgb(g, scale),
    scaleAndClampRgb(b, scale)
  };
}

float mapLabToXyzComponent(float t) {
  return t > 0.206896552 ? t * t * t : 0.12841855 * (t - 0.137931034);
}

RGB LCH::toRGB() const {
  // Convert to L*a*b*
  const float a = c * cosf(h * M_PI_180);
  const float b = c * sinf(h * M_PI_180);
  /*
  Serial.print("l=");
  Serial.print(l);
  Serial.print(", c=");
  Serial.print(c);
  Serial.print(", h=");
  Serial.print(h);
  Serial.print(", a=");
  Serial.print(a);
  Serial.print(", b=");
  Serial.print(b);
  */
  
  // Convert to XYZ D65 then linear RGB
  // Based on: https://github.com/gka/chroma.js/blob/b58c6d04b2579bb99d2b07b73a4496ef20de9da1/chroma.js#L1162
  const float y = (l + 16) / 116;
  const float x = y + a / 500;
  const float z = y - b / 200;
  const float yy = mapLabToXyzComponent(y);
  const float xx = mapLabToXyzComponent(x) * 0.950470;
  const float zz = mapLabToXyzComponent(z) * 1.088830;
  const float rr = 3.2404542 * xx - 1.5371385 * yy - 0.4985314 * zz;
  const float gg = -0.9692660 * xx + 1.8760108 * yy + 0.0415560 * zz;
  const float bb = 0.0556434 * xx - 0.2040259 * yy + 1.0572252 * zz;

/*
  Serial.print(", x=");
  Serial.print(x);
  Serial.print(", y=");
  Serial.print(y);
  Serial.print(", z=");
  Serial.print(z);
  Serial.print(", xx=");
  Serial.print(xx);
  Serial.print(", yy=");
  Serial.print(yy);
  Serial.print(", zz=");
  Serial.print(zz);
  Serial.print(", rr=");
  Serial.print(rr);
  Serial.print(", gg=");
  Serial.print(gg);
  Serial.print(", bb=");
  Serial.print(bb);
  Serial.println();
*/
  return RGB{scaleAndClampRgb(rr, 255.f), scaleAndClampRgb(gg, 255.f), scaleAndClampRgb(bb, 255.f)};
}

void printTint(Print& printer, tint_t tint) {
  if (tint == TINT_WHITE) {
    printer.print("White");
  } else {
    printer.print(tint);
  }
}

void printTone(Print& printer, tint_t tint, tone_t tone) {
  if (!tintHasTone(tint)) {
    printer.print("n/a");
  } else {
    printer.print(tone);
  }
}

void printBrightness(Print& printer, brightness_t brightness) {
  if (brightness == BRIGHTNESS_OFF) {
    printer.print("Off");
  } else {
    printer.print(brightness);
  }
}

RGB makeKnobColor(tint_t tint, tone_t tone, brightness_t brightness) {
  const float scale = brightness * 0.1f;
#if USE_LCH_COLOR
  if (tint == TINT_WHITE) {
    return LCH{100.f, 0.0f, 0.0f}.toRGB() * scale;
  } else {
    return LCH{60.f, 70.f, (tint - 1) * 10.f}.toRGB() * scale;
  }
#else
  if (tint == TINT_WHITE) {
    return RGB{
      scaleAndClampRgb(253.f, scale),
      scaleAndClampRgb(244.f, scale),
      scaleAndClampRgb(220.f, scale)
    };
  } else {
    const uint8_t pos = uint32_t(tint) * 255 / 36;
    const RGB color = RGB::colorWheel(pos);
    const float alpha = tone * 0.1f;
    const float beta = 1.f - alpha;
    return RGB{
      scaleAndClampRgb(color.r * alpha + 255.f * beta, scale),
      scaleAndClampRgb(color.g * alpha + 255.f * beta, scale),
      scaleAndClampRgb(color.b * alpha + 255.f * beta, scale)
    };
  }
#endif
}

RGBW makeStripColor(tint_t tint, tone_t tone, brightness_t brightness) {
  // make scale non-linear to expand dynamic range at low end
  const float scale = powf(brightness * 0.1f, 2.5f);
#if USE_LCH_COLOR
  if (tint == TINT_WHITE) {
    return RGBW{0, 0, 0, 255} * scale;
  } else {
    RGB rgb = LCH{0.3f, 100.f, (tint - 1) * 10.f}.toRGB();
    return RGBW{rgb.r, rgb.g, rgb.b, 180} * scale;
  }
#else
  if (tint == TINT_WHITE) {
    return RGBW{0, 0, 0, scaleAndClampRgb(210.f, scale)};
  } else {
    const uint8_t pos = uint32_t(tint) * 255 / 36;
    const RGB color = RGB::colorWheel(pos);
    const float alpha = 0.4f + tone * 0.06f;
    const float beta = 0.6f - tone * 0.06f;
    return RGBW{
      scaleAndClampRgb(color.r * alpha, scale),
      scaleAndClampRgb(color.g * alpha, scale),
      scaleAndClampRgb(color.b * alpha, scale),
      scaleAndClampRgb(255.f * beta, scale),
    };
  }
#endif
}
