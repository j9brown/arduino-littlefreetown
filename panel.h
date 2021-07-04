/*
   Driver for the FYSETC v2.1 RGB Mini 12864 Panel.

   Supports LCD, RGB LEDs, encoder wheel, knobs, and beeper.
   Does not support SDCARD accessory.

   References:

   - https://github.com/FYSETC/FYSETC-Mini-12864-Panel
   - https://www.pjrc.com/store/teensy32.html

   Required libraries:
 
   - Adafruit_NeoPixel
   - MD_REncoder
   - Switch
   - U8g2

   Wiring for Teensy 3.2:

   - EXP1-1 (VCC)     -> any Vin
   - EXP1-2 (GND)     -> any GND
   - EXP1-3           -> N/C
   - EXP1-4           -> N/C
   - EXP1-5 (LED_DIN) -> pin 4 (PWM), 3.3 or 5 V, output
   - EXP1-6 (LCD_RST) -> pin 5, 3.3 V, output
   - EXP1-7 (LCD_A0)  -> pin 6, 3.3 V, output
   - EXP1-8 (LCD_CS)  -> pin 15 (CS-ALT), 3.3 V, output
   - EXP1-9 (BTN_ENC) -> pin 8, 3.3 V, input-pullup, active low
   - EXP1-10 (BEEP)   -> pin 9 (PWM), 3.3 or 5 V, output
   - EXP2-1 (SW_KILL) -> pin 2, 5 V, input, active low, board has pull up to VCC
   - EXP2-2 (GND)     -> any GND
   - EXP2-3 (SW_RST)  -> N/C
   - EXP2-4 (CD)      -> N/C
   - EXP2-5 (MOSI)    -> pin 7 (DOUT-ALT), 3.3 V, output
   - EXP2-6 (BTN_EN2) -> pin 17, 3.3 V, input-pullup
   - EXP2-7 (SS)      -> N/C
   - EXP2-8 (BTN_EN1) -> pin 16, 3.3 V, input-pullup
   - EXP2-9 (SCK)     -> pin 14 (SCK-ALT), 3.3 V, output
   - EXP2-10 (MISO)   -> N/C
*/

#pragma once

#include <Adafruit_NeoPixel.h>
#include <Arduino.h>
#include <avdweb_Switch.h>
#include <MD_REncoder.h>
#include <U8g2lib.h>

#include "utils.h"

class Panel {
public:
  enum class ButtonEvent {
    NONE = 0,
    LONG_PRESS,
    SINGLE_CLICK,
    DOUBLE_CLICK
  };

  Panel();
  ~Panel() = default;

  // Initialize the panel.
  void begin();

  // Update the panel state.
  void update();

  // Returns the number of steps that the rotary encoder has turned since
  // the last call to read. Values greater than 0 indicate clockwise rotations.
  // Values less than 0 indicate counter-clockwise rotations.
  int32_t readKnobRotations();

  // Returns the knob button's most recent event and clears its state.
  ButtonEvent readKnobButton();

  // Returns the kill button's most recent event and clears its state.
  ButtonEvent readKillButton();

  // Gets the display's drawing interface.
  inline U8G2& gfx() { return _display; }

  // Sets the panel's colors.
  void setColors(RGB display, RGB knob);

  // Plays a tone of finite duration to the buzzer.
  void playTone(uint32_t freq, uint32_t millis);

private:
  Panel(const Panel&) = delete;
  Panel(Panel&&) = delete;  
  Panel& operator=(const Panel&) = delete;
  Panel& operator=(Panel&&) = delete;

  static void handleKnobRotation();
  static void updateButton(Switch* button, volatile ButtonEvent* event);

  static Panel* _self; // for use in interrupt contexts

  U8G2_ST7567_OS12864_F_4W_HW_SPI _display;
  Adafruit_NeoPixel _leds;

  MD_REncoder _knobEncoder;
  volatile int32_t _knobRotations;

  Switch _knobButton;
  volatile ButtonEvent _knobButtonEvent;

  Switch _killButton;
  volatile ButtonEvent _killButtonEvent;
};
