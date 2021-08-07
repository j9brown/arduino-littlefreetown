#include <SPI.h>

#include "panel.h"
#include "utils.h"

namespace {
const u8g2_cb_t* LCD_ROTATION = U8G2_R2;
const int LCD_CS = 15;
const int LCD_A0 = 6;
const int LCD_RST = 5;
const int LCD_MOSI = 7;
const int LCD_SCK = 14;

const int LED_DIN = 4;

const int SW_KILL = 2;

const int BEEP = 9;

const int BTN_ENC = 8;
const int BTN_EN1 = 16;
const int BTN_EN2 = 17;
} // namespace

Panel* Panel::_self;

Panel::Panel() :
    _display(LCD_ROTATION, LCD_CS, LCD_A0, LCD_RST),
    _leds(3, LED_DIN, NEO_RGB | NEO_KHZ800),
    _knobEncoder(BTN_EN2, BTN_EN1),
    _knobRotations(0),
    _knobButton(BTN_ENC, INPUT_PULLUP),
    _killButton(SW_KILL, INPUT) {
}

void Panel::begin() {
  assert(_self == nullptr);
  _self = this;

  // Reassign the SPI pins
  SPI.setMOSI(LCD_MOSI);
  SPI.setSCK(LCD_SCK);

  // Initialize the display
  _display.begin();

  // Initialize the LEDs
  _leds.begin();
  _leds.show();

  // Initialize the rotary encoder
  _knobEncoder.begin();
  attachInterrupt(digitalPinToInterrupt(BTN_EN1), handleKnobRotation, CHANGE);
  attachInterrupt(digitalPinToInterrupt(BTN_EN2), handleKnobRotation, CHANGE);
}

void Panel::update() {
  // TODO: move to an ISR
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
  _knobRotations -= count;
  return count;
}

void Panel::handleKnobRotation() {
  switch (_self->_knobEncoder.read()) {
    case DIR_CW:
      _self->_knobRotations++;
      break;
    case DIR_CCW:
      _self->_knobRotations--;
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

void Panel::updateButton(Switch* button, volatile ButtonEvent* event) {
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
