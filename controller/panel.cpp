#include <SPI.h>

#include "panel.h"
#include "utils.h"

namespace {
//const u8g2_cb_t* LCD_ROTATION = U8G2_R2;
const u8g2_cb_t* LCD_ROTATION = U8G2_R0;
const int LCD_CS = 9;
const int LCD_A0 = 8;
const int LCD_RST = 7;
const int LCD_MOSI = 11;
const int LCD_MISO = 12;
const int LCD_SCK = 14;

const int LED_DIN = 5;

const int SW_KILL = 18;

const int BEEP = 6;

const int BTN_ENC = 19;
const int BTN_EN1 = 15;
const int BTN_EN2 = 16;
} // namespace

Panel::Panel() :
    _display(LCD_ROTATION, LCD_CS, LCD_A0, LCD_RST),
    _leds(3, LED_DIN, NEO_RGB | NEO_KHZ800),
    _knobEncoder(BTN_EN2, BTN_EN1),
    _knobRotations(0),
    _knobButton(BTN_ENC, INPUT_PULLUP),
    _killButton(SW_KILL, INPUT) {
}

void Panel::begin(SnoozeDigital& snoozeDigital) {
  // Reassign the SPI pins
  SPI.setMOSI(LCD_MOSI);
  SPI.setMISO(LCD_MISO);
  SPI.setSCK(LCD_SCK);

  // Initialize the display
  _display.begin();

  // Initialize the LEDs
  _leds.begin();
  _leds.show();

  // Initialize the rotary encoder
  _knobEncoder.begin();

  // Configure snooze block
  snoozeDigital.pinMode(BTN_ENC, INPUT_PULLUP, CHANGE);
  snoozeDigital.pinMode(SW_KILL, INPUT, CHANGE);
  snoozeDigital.pinMode(BTN_EN1, INPUT_PULLUP, CHANGE);
  snoozeDigital.pinMode(BTN_EN2, INPUT_PULLUP, CHANGE);
}

void Panel::update() {
  updateKnobRotation();
  updateButton(&_knobButton, &_knobButtonEvent);
  updateButton(&_killButton, &_killButtonEvent);
}

void Panel::setColors(RGB display, RGB knob) {
  _leds.setPixelColor(0, display.r, display.g, display.b);
  _leds.setPixelColor(1, knob.r, knob.g, knob.b);
  _leds.setPixelColor(2, knob.r, knob.g, knob.b);
  _leds.show();
}

int32_t Panel::readKnobRotations() {
  int32_t count = _knobRotations;
  _knobRotations = 0;
  return count;
}

void Panel::updateKnobRotation() {
  switch (_knobEncoder.read()) {
    case DIR_CW:
      _knobRotations++;
      break;
    case DIR_CCW:
      _knobRotations--;
      break;
  }
}

Panel::ButtonEvent Panel::readKnobButton() {
  ButtonEvent event = _knobButtonEvent;
  _knobButtonEvent = ButtonEvent::NONE;
  return event;
}

Panel::ButtonEvent Panel::readKillButton() {
  ButtonEvent event = _killButtonEvent;
  _killButtonEvent = ButtonEvent::NONE;
  return event;
}

void Panel::updateButton(Switch* button, ButtonEvent* event) {
  button->poll();
  if (button->doubleClick()) {
    *event = ButtonEvent::DOUBLE_CLICK;
  } else if (button->singleClick()) {
    *event = ButtonEvent::SINGLE_CLICK;
  } else if (button->longPress()) {
    *event = ButtonEvent::LONG_PRESS;
  }
}

void Panel::playTone(uint32_t freq, uint32_t millis) {
  tone(BEEP, freq, millis);
}

bool Panel::canSleep() const {
  return _knobRotations == 0
      && _knobButtonEvent == ButtonEvent::NONE
      && _killButtonEvent == ButtonEvent::NONE
      && digitalRead(BTN_ENC)
      && digitalRead(SW_KILL);
}
