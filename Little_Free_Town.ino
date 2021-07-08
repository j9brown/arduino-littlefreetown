/*
 * Wiring:
 * - see panel.3
 * - pin 3: LEDs
 */

#include <memory>

#include <Adafruit_NeoPixel.h>
#include <Adafruit_SleepyDog.h>
#include <Print.h>
#include <TimeLib.h>

#include "panel.h"
#include "settings.h"
#include "ui.h"
#include "utils.h"

namespace {
const uint32_t SETTINGS_SCHEMA_VERSION = 2;
Setting<uint8_t, 0> activityTimeoutSeconds;
Setting<uint8_t, 1> dawnHour;
Setting<uint8_t, 2> duskHour;
Setting<uint8_t, 3> nightHour;
Setting<brightness_t, 4> daytimeBrightness;
Setting<brightness_t, 5> eveningBrightness;
Setting<brightness_t, 6> nighttimeBrightness;
Setting<tint_t, 100> museumLightTint;
Setting<brightness_t, 101> museumLightBrightness;
Setting<tint_t, 200> libraryLightTint;
Setting<brightness_t, 201> libraryLightBrightness;
Setting<uint8_t, 2000> testSetting1;
Setting<int8_t, 2001> testSetting2;

void resetSettings() {
  activityTimeoutSeconds.set(30);
  dawnHour.set(7);
  duskHour.set(18);
  nightHour.set(23);
  daytimeBrightness.set(BRIGHTNESS_MAX);
  eveningBrightness.set(BRIGHTNESS_MAX);
  nighttimeBrightness.set(BRIGHTNESS_MAX);
  museumLightTint.set(TINT_WHITE);
  museumLightBrightness.set(BRIGHTNESS_MAX);
  libraryLightTint.set(TINT_WHITE);
  libraryLightBrightness.set(BRIGHTNESS_MAX);
  testSetting1.set(0);
  testSetting2.set(0);
}

Settings settings;

Panel panel;
Binding binding(&panel);
Stage stage(&binding,
  [] { return activityTimeoutSeconds.get() * 1000UL; });

constexpr int LIGHTS_PIN = 3;
constexpr unsigned LIGHTS_MUSEUM_FIRST = 0;
constexpr unsigned LIGHTS_MUSEUM_COUNT = 11;
constexpr unsigned LIGHTS_LIBRARY_FIRST = 11;
constexpr unsigned LIGHTS_LIBRARY_COUNT = 11;
constexpr unsigned LIGHTS_TOTAL_COUNT = 22;
Adafruit_NeoPixel lights(LIGHTS_TOTAL_COUNT, LIGHTS_PIN, NEO_GRBW | NEO_KHZ800);
RGBW oldLights[LIGHTS_TOTAL_COUNT];
RGBW newLights[LIGHTS_TOTAL_COUNT];

enum class TimeOfDay {
  NIGHTTIME,
  DAYTIME,
  EVENING
};

TimeOfDay timeOfDay() {
  uint8_t h = hour();
  if (h < dawnHour.get()) {
    if (nightHour.get() < dawnHour.get() && h < nightHour.get()) {
      return TimeOfDay::EVENING;
    }
    return TimeOfDay::NIGHTTIME;
  }
  if (h < duskHour.get()) {
    return TimeOfDay::DAYTIME;
  }
  if (h < nightHour.get()) {
    return TimeOfDay::EVENING;
  }
  return TimeOfDay::NIGHTTIME;
}

brightness_t brightnessForTimeOfDay() {
  switch (timeOfDay()) {
    case TimeOfDay::DAYTIME:
      return daytimeBrightness.get();
    case TimeOfDay::EVENING:
      return eveningBrightness.get();
    case TimeOfDay::NIGHTTIME:
      return nighttimeBrightness.get();
  }
  return 1.0f;
}
} // namespace

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
  while (day(makeTime(*te)) != te->Day) {
    te->Day = rollover ? 1 : te->Day - 1;
  }
}

std::unique_ptr<Menu> makeMuseumMenu() {
  auto menu = std::make_unique<Menu>();
  menu->addItem(std::make_unique<Item>("* MUSEUM *"));
  menu->addItem(std::make_unique<TintItem>("Light Tint", museumLightTint));
  menu->addItem(std::make_unique<BrightnessItem>("Light Brightness", museumLightBrightness));
  return menu;
}

std::unique_ptr<Menu> makeLibraryMenu() {
  auto menu = std::make_unique<Menu>();
  menu->addItem(std::make_unique<Item>("* LIBRARY *"));
  menu->addItem(std::make_unique<TintItem>("Light Tint", libraryLightTint));
  menu->addItem(std::make_unique<BrightnessItem>("Light Brightness", libraryLightBrightness));
  return menu;
}

std::unique_ptr<Menu> makeTimeMenu() {
  auto menu = std::make_unique<Menu>();
  menu->addItem(std::make_unique<Item>("* TIME *"));
  menu->addItem(std::make_unique<NumericItem<int>>("Year",
    [] { return year(); },
    [] (int x) {
      editTime([x] (TimeElements* te) {
        te->Year = CalendarYrToTm(x);
        clampDayOfMonth(te, false);
      });
    },
    2021, 2037, 1));
  menu->addItem(std::make_unique<NumericItem<int>>("Month",
    [] { return month(); },
    [] (int x) {
      editTime([x] (TimeElements* te) {
        te->Month = x;
        clampDayOfMonth(te, false);
      });
    },
    1, 12, 1));
  menu->addItem(std::make_unique<NumericItem<int>>("Day",
    [] { return day(); },
    [] (int x) {
      editTime([x] (TimeElements* te) {
        bool rollover = x > te->Day;
        te->Day = x;
        clampDayOfMonth(te, rollover);
      });
    },
    1, 31, 1));
  menu->addItem(std::make_unique<NumericItem<int>>("Hour",
    [] { return hour(); },
    [] (int x) {
      editTime([x] (TimeElements* te) { te->Hour = x; });
    },
    0, 23, 1));
  menu->addItem(std::make_unique<NumericItem<int>>("Minute",
    [] { return minute(); },
    [] (int x) {
      editTime([x] (TimeElements* te) { te->Minute = x; });
    },
    0, 59, 1));
  menu->addItem(std::make_unique<NumericItem<int>>("Second",
    [] { return second(); },
    [] (int x) {
      editTime([x] (TimeElements* te) { te->Second = x; });
    },
    0, 59, 1));
  return menu;
}

std::unique_ptr<Menu> makePowerSavingMenu() {
  auto menu = std::make_unique<Menu>();
  menu->addItem(std::make_unique<Item>("* POWER *"));
  menu->addItem(std::make_unique<NumericItem<uint8_t>>("Dawn Hour",
    dawnHour, 0, 23, 1));
  menu->addItem(std::make_unique<NumericItem<uint8_t>>("Dusk Hour",
    duskHour, 0, 23, 1));
  menu->addItem(std::make_unique<NumericItem<uint8_t>>("Night Hour",
    nightHour, 0, 23, 1));
  menu->addItem(std::make_unique<BrightnessItem>("Daytime Brightness",
    daytimeBrightness));
  menu->addItem(std::make_unique<BrightnessItem>("Evening Brightness",
    eveningBrightness));
  menu->addItem(std::make_unique<BrightnessItem>("Nighttime Brightness",
    nighttimeBrightness));
  menu->addItem(std::make_unique<NumericItem<uint8_t>>("Display Timeout (s)",
    activityTimeoutSeconds, 0, 240, 10));
  return menu;
}

class BoardTest : public Scene {
public:
  BoardTest() {}
  virtual ~BoardTest() override {}

  void poll(Context& context) override;
  void draw(Context& context, Canvas& canvas) override;
  bool input(Context& context, const InputEvent& event) override;

private:
  String _message = "---";
  uint8_t _index = 0;
  uint8_t _values[2] = { 200, 40 };
  time_t _time;
};

void BoardTest::poll(Context& context) {
  time_t t = now();
  if (t != _time) {
    _time = t;
    context.requestDraw();
  }
}

bool BoardTest::input(Context& context, const InputEvent& event) {
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

void BoardTest::draw(Context& context, Canvas& canvas) {
  canvas.gfx().drawStr(1, 0, "* BOARD TEST *");

  canvas.gfx().drawStr(1, 20, _message.c_str());

  canvas.gfx().setCursor(1, 40);
  canvas.gfx().print("Time: ");
  printDateAndTime(canvas.gfx(), _time);

  canvas.gfx().setCursor(1, 50);
  canvas.gfx().print("Time of Day: ");
  switch (timeOfDay()) {
    case TimeOfDay::DAYTIME:
      canvas.gfx().print("Daytime");
      break;
    case TimeOfDay::EVENING:
      canvas.gfx().print("Evening");
      break;
    case TimeOfDay::NIGHTTIME:
      canvas.gfx().print("Nighttime");
      break;
  }

  canvas.setDisplayColor(colorWheel(_values[0])* 0.8f);
  canvas.setKnobColor(colorWheel(_values[1]) * 0.4f);
}

std::unique_ptr<BoardTest> makeBoardTestScene() {
  return std::make_unique<BoardTest>();
}

std::unique_ptr<Menu> makeEepromTestMenu() {
  auto menu = std::make_unique<Menu>();
  menu->addItem(std::make_unique<Item>("* EEPROM TEST *"));
  menu->addItem(std::make_unique<NumericItem<uint8_t>>("Test Setting 1",
    testSetting1, 0, 100, 5));
  menu->addItem(std::make_unique<NumericItem<int8_t>>("Test Setting 2",
    testSetting2, -10, 10, 1));
  return menu;
}

std::unique_ptr<Menu> makeFactoryResetMenu() {
  auto menu = std::make_unique<Menu>();
  menu->addItem(std::make_unique<Item>("* FACTORY RESET *"));
  menu->addItem(std::make_unique<BackItem>("Back Away Slowly..."));
  menu->addItem(std::make_unique<NavigateItem>("Erase All Settings!", Settings::eraseAndReboot));
  return menu;
}

std::unique_ptr<Menu> makeDiagnosticsMenu() {
  auto menu = std::make_unique<Menu>();
  menu->addItem(std::make_unique<Item>("* DIAGNOSTICS *"));
  menu->addItem(std::make_unique<NavigateItem>("Board Test", makeBoardTestScene));
  menu->addItem(std::make_unique<NavigateItem>("EEPROM Test", makeEepromTestMenu));
  menu->addItem(std::make_unique<NavigateItem>("Factory Reset", makeFactoryResetMenu));
  return menu;
}

std::unique_ptr<Menu> makeRootMenu() {
  auto menu = std::make_unique<Menu>();
  menu->addItem(std::make_unique<Item>("* LITTLE FREE TOWN *"));
  menu->addItem(std::make_unique<NavigateItem>("Museum", makeMuseumMenu));
  menu->addItem(std::make_unique<NavigateItem>("Library", makeLibraryMenu));
  menu->addItem(std::make_unique<NavigateItem>("Time", makeTimeMenu));
  menu->addItem(std::make_unique<NavigateItem>("Power Saving", makePowerSavingMenu));
  menu->addItem(std::make_unique<NavigateItem>("Diagnostics", makeDiagnosticsMenu));
  return menu;
}

void renderMuseumLights(float scale) {
  RGBW color = makeStripColor(museumLightTint.get(), museumLightBrightness.get()) * scale;
  for (size_t i = 0; i < LIGHTS_MUSEUM_COUNT; i++) {
    newLights[i + LIGHTS_MUSEUM_FIRST] = color;
  }  
}

void renderLibraryLights(float scale) {
  RGBW color = makeStripColor(libraryLightTint.get(), libraryLightBrightness.get()) * scale;
  for (size_t i = 0; i < LIGHTS_LIBRARY_COUNT; i++) {
    newLights[i + LIGHTS_LIBRARY_FIRST] = color;
  }  
}

void updateLights() {
  const float scale = brightnessForTimeOfDay() * 0.1f;
  renderMuseumLights(scale);
  renderLibraryLights(scale);

  bool changed = false;
  for (size_t i = 0; i < LIGHTS_TOTAL_COUNT; i++) {
    changed |= oldLights[i] != newLights[i];
  }
  if (changed) {
    for (size_t i = 0; i < LIGHTS_TOTAL_COUNT; i++) {
      oldLights[i] = newLights[i];
      lights.setPixelColor(i, newLights[i].r, newLights[i].g, newLights[i].b, newLights[i].w);
    }
    lights.show();
  }  
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

  // Print welcome message.
  Serial.println("Little Free Town\n");
  Serial.print("Time: ");
  printDateAndTime(Serial, now());
  Serial.println();

  // Initialize the settings.
  settings.begin(SETTINGS_SCHEMA_VERSION, resetSettings);

  // Initialize the rest of the hardware.
  panel.begin();
  stage.begin(makeRootMenu());
  lights.begin();
}

void loop() {
  Watchdog.reset();
  
  panel.update();
  stage.update();
  updateLights();
}
