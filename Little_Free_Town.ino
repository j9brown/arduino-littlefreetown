#include <memory>

#include <Adafruit_SleepyDog.h>
#include <Print.h>
#include <TimeLib.h>

#include "panel.h"
#include "settings.h"
#include "ui.h"

namespace {
Settings settings;
Panel panel;
Binding binding(&panel);
Stage stage(&binding);

const uint32_t SETTINGS_SCHEMA_VERSION = 1;
Setting<bool, 0> alwaysOn;
} // namespace

void printTime(Print& printer) {
  TimeElements time;
  breakTime(now(), time);
  printer.print("Time: ");
  printer.print(time.Year + 1970);
  printer.print('/');
  printer.print(time.Month);
  printer.print('/');
  printer.print(time.Day);
  printer.print(' ');
  printer.print(time.Hour);
  printer.print(':');
  if (time.Minute < 10) printer.print('0');
  printer.print(time.Minute);
  printer.print(':');
  if (time.Second < 10) printer.print('0');
  printer.print(time.Second);
  printer.println(); 
}

void printWelcome() {
  Serial.println("Little Free Town\n");
  if (timeStatus() == timeSet) {
    Serial.println("Time was synchronized with the host PC");
  }
  printTime(Serial);
}

class SelfTest : public Scene {
public:
  SelfTest() {}
  virtual ~SelfTest() override {}

  void draw(Context& context, Canvas& canvas) override;
  bool input(Context& context, const InputEvent& event) override;

private:
  String _message = "---";
  uint8_t _index = 0;
  uint8_t _values[2] = { 200, 40 };
};

void SelfTest::draw(Context& context, Canvas& canvas) {
  canvas.gfx().drawStr(0, 10, "Hello world!");
  canvas.gfx().drawStr(0, 30, _message.c_str());
  canvas.gfx().setCursor(0, 50);
  printTime(canvas.gfx());
  canvas.setDisplayColor(colorWheel(_values[0])* 0.8f);
  canvas.setKnobColor(colorWheel(_values[1]) * 0.4f);
}

bool SelfTest::input(Context& context, const InputEvent& event) {
  switch (event.type) {
    case InputType::LONG_PRESS:
      _message = "LONG_PRESS";
      context.requestSleep();
      break;
    case InputType::SINGLE_CLICK:
      _message = "SINGLE_CLICK";
      _index = _index ? 0 : 1;
      break;
    case InputType::DOUBLE_CLICK:
      _message = "DOUBLE_CLICK";
      break;
    case InputType::ROTATE:
      _message = "ROTATE: " + String(event.value);
      _values[_index] += uint8_t(event.value * 5);
      break;
    case InputType::BACK:
      _message = "BACK";
      break;
    case InputType::HOME:
      _message = "HOME";
      break;
    default:
      break;
  }

  context.requestDraw();
  Serial.println(_message);
  return false;
}

void setup() {
  // Enable the watchdog timer with 1 second timeout
  // Doesn't seem to work (reset call doesn't actually reset watchdog)
  //Watchdog.enable(1000);
  
  // Configure the time library to use the hardware RTC
  setSyncProvider([]() -> time_t { return Teensy3Clock.get(); } );

  // Wait briefly for the serial monitor to connect for debugging.
  Serial.begin(115200);
  for (int i = 0; !Serial && i < 10; i++) {
    delay(100);
  }
  printWelcome();

  // Initialize the rest of the hardware.
  settings.begin(SETTINGS_SCHEMA_VERSION);
  panel.begin();
  stage.begin(std::make_unique<SelfTest>());
}

void loop() {
  Watchdog.reset();
  
  panel.update();
  stage.update();
}
