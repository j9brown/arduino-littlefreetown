#include "utils.h"

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

RGB colorWheel(uint8_t pos) {
  pos = 255 - pos;
  if (pos < 85)
    return RGB{uint8_t(255 - pos * 3), 0, uint8_t(pos * 3)};
  if (pos < 170)
    return RGB{0, uint8_t((pos - 85) * 3), uint8_t(255 - (pos - 85) * 3)};
  return RGB{uint8_t((pos - 170) * 3), uint8_t(255 - (pos - 170) * 3), 0};
}
