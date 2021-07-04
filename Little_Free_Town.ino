#include <memory>

#include <Adafruit_SleepyDog.h>
#include <Print.h>
#include <TimeLib.h>

#include "panel.h"
#include "settings.h"
#include "ui.h"
#include "utils.h"

namespace {
Settings settings;
Panel panel;
Binding binding(&panel);
Stage stage(&binding);

const uint32_t SETTINGS_SCHEMA_VERSION = 1;
Setting<bool, 0> alwaysOn;
} // namespace

class SelfTest : public Scene {
public:
  SelfTest() {}
  virtual ~SelfTest() override {}

  void poll(Context& context) override;
  void draw(Context& context, Canvas& canvas) override;
  bool input(Context& context, const InputEvent& event) override;

private:
  String _message = "---";
  uint8_t _index = 0;
  uint8_t _values[2] = { 200, 40 };
  time_t _time;
};

void SelfTest::poll(Context& context) {
  time_t t = now();
  if (t != _time) {
    _time = t;
    context.requestDraw();
  }
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

void SelfTest::draw(Context& context, Canvas& canvas) {
  canvas.gfx().drawStr(1, 0, "* SELF TEST *");
  canvas.gfx().drawStr(1, 20, _message.c_str());
  canvas.gfx().setCursor(1, 50);
  canvas.gfx().print("Time: ");
  printDateAndTime(canvas.gfx(), _time);
  canvas.setDisplayColor(colorWheel(_values[0])* 0.8f);
  canvas.setKnobColor(colorWheel(_values[1]) * 0.4f);
}

std::unique_ptr<SelfTest> makeSelfTest() {
  return std::make_unique<SelfTest>();
}

template <typename Fn>
void editTime(Fn fn) {
  time_t time = now();
  TimeElements te;
  breakTime(time, te);
  fn(&te);
  time = makeTime(te);
  Teensy3Clock.set(time);
  setTime(time);
}

void clampDayOfMonth(TimeElements* te, bool rollover) {
  while (month(makeTime(*te)) != te->Month) {
    te->Day = rollover ? 1 : te->Day - 1;
  }
}

std::unique_ptr<Menu> makeTimeMenu() {
  auto menu = std::make_unique<Menu>();
  menu->addItem(std::make_unique<Item>("* TIME *"));
  menu->addItem(std::make_unique<NumberItem<int>>("Year",
    [] { return year(); },
    [] (int x) {
      editTime([x] (TimeElements* te) {
        te->Year = CalendarYrToTm(x < 2021 ? 2037 : x > 2037 ? 2021 : x);
        clampDayOfMonth(te, false);
      });
    },
    2020, 2038, 1));
  menu->addItem(std::make_unique<NumberItem<int>>("Month",
    [] { return month(); },
    [] (int x) {
      editTime([x] (TimeElements* te) {
        te->Month = x < 1 ? 12 : x > 12 ? 1 : x;
        clampDayOfMonth(te, false);
      });
    },
    0, 13, 1));
  menu->addItem(std::make_unique<NumberItem<int>>("Day",
    [] { return day(); },
    [] (int x) {
      editTime([x] (TimeElements* te) {
        te->Day = x < 1 ? 31 : x > 31 ? 1 : x;
        clampDayOfMonth(te, x > 0);
      });
    },
    0, 32, 1));
  menu->addItem(std::make_unique<NumberItem<int>>("Hour",
    [] { return hour(); },
    [] (int x) {
      editTime([x] (TimeElements* te) {
        te->Hour = x < 0 ? 23 : x > 23 ? 0 : x;
      });
    },
    -1, 24, 1));
  menu->addItem(std::make_unique<NumberItem<int>>("Minute",
    [] { return minute(); },
    [] (int x) {
      editTime([x] (TimeElements* te) {
        te->Minute = x < 0 ? 59 : x > 59 ? 0 : x;
      });
    },
    -1, 60, 1));
  menu->addItem(std::make_unique<NumberItem<int>>("Second",
    [] { return second(); },
    [] (int x) {
      editTime([x] (TimeElements* te) {
        te->Second = x < 0 ? 59 : x > 59 ? 0 : x;
      });
    },
    -1, 60, 1));
  return menu;
}

int32_t testSignedValue = 42;
uint32_t testUnsignedValue = 33;

std::unique_ptr<Menu> makeRootMenu() {
  auto menu = std::make_unique<Menu>();
  menu->addItem(std::make_unique<Item>("* LITTLE FREE TOWN *"));
  menu->addItem(std::make_unique<NavigateItem>("Self Test", makeSelfTest));
  menu->addItem(std::make_unique<NavigateItem>("Time", makeTimeMenu));
  menu->addItem(std::make_unique<NumberItem<int32_t>>("Signed",
    [] { return testSignedValue; },
    [] (int32_t x) { testSignedValue = x; },
    -100, 100, 3));
  menu->addItem(std::make_unique<NumberItem<uint32_t>>("Unsigned",
    [] { return testUnsignedValue; },
    [] (int32_t x) { testUnsignedValue = x; },
    0, 100, 2));
  return menu;
}

void printWelcome() {
  Serial.println("Little Free Town\n");
  if (timeStatus() == timeSet) {
    Serial.println("Time was synchronized with the host PC");
  }
  Serial.print("Time: ");
  printDateAndTime(Serial, now());
  Serial.println();
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
  stage.begin(makeRootMenu());
}

void loop() {
  Watchdog.reset();
  
  panel.update();
  stage.update();
}
