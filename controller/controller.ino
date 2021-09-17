/*
 * Required libraries:
 *
 * - Snooze
 *
 * Wiring:
 * - see panel.h
 * - pin 0: DOOR1
 * - pin 1: DOOR2
 * - pin 2: STRIP_LED_EN
 * - pin 3: CHG
 * - pin 4: PGOOD
 * - pin 20/A6: BATTMON
 *           100 K resistor to Vbat
 *           100 K resistor to GND
 *           100 nF capacitor to GND
 * - pin 21: STRIP_LED_DIN
 * - cut trace between Vin and USB if using battery
 */

#include <algorithm>
#include <memory>

#include <Adafruit_NeoPixel.h>
#include <Snooze.h>
#include <SnoozeBlock.h>
#include <Print.h>
#include <TimeLib.h>

#include "panel.h"
#include "settings.h"
#include "ui.h"
#include "utils.h"

#define USE_BUILTIN_LED 0

enum class StrandTestPattern : uint8_t {
  DISABLED, WHITE, GLOW, RAINBOW
};

template <>
struct ChoiceTraits<StrandTestPattern> {
  static constexpr StrandTestPattern min = StrandTestPattern::DISABLED;
  static constexpr StrandTestPattern max = StrandTestPattern::RAINBOW;
  
  static const char* toString(StrandTestPattern value) {
    switch (value) {
      default:
      case StrandTestPattern::DISABLED: return "Disabled";
      case StrandTestPattern::WHITE: return "White";
      case StrandTestPattern::GLOW: return "Glow";
      case StrandTestPattern::RAINBOW: return "Rainbow";
    }
  }
};

enum class OnOff : uint8_t {
  OFF, ON
};

template <>
struct ChoiceTraits<OnOff> {
  static constexpr OnOff min = OnOff::OFF;
  static constexpr OnOff max = OnOff::ON;
  
  static const char* toString(OnOff value) {
    switch (value) {
      default:
      case OnOff::OFF: return "Off";
      case OnOff::ON: return "On";
    }
  }
};

enum class LowBattery : uint8_t {
  DISABLED, V3_3, V3_4, V3_5, V3_6
};

template <>
struct ChoiceTraits<LowBattery> {
  static constexpr LowBattery min = LowBattery::DISABLED;
  static constexpr LowBattery max = LowBattery::V3_6;
  
  static const char* toString(LowBattery value) {
    switch (value) {
      default:
      case LowBattery::DISABLED: return "Disabled";
      case LowBattery::V3_3: return "3.3 V";
      case LowBattery::V3_4: return "3.4 V";
      case LowBattery::V3_5: return "3.5 V";
      case LowBattery::V3_6: return "3.6 V";
    }
  }
};

enum class LightState {
  OFF, ON, ANIMATING
};

LightState mergeLightState(LightState a, LightState b) {
  if (a == LightState::ANIMATING || b == LightState::ANIMATING)
      return LightState::ANIMATING;
  if (a == LightState::ON || b == LightState::ON)
      return LightState::ON;
  return LightState::OFF;
}

namespace {
using vbat_t = uint8_t;
constexpr time_t BATTERY_HISTORY_INTERVAL = 60 * 15; // sample every 15 minutes
constexpr unsigned BATTERY_HISTORY_LENGTH = 256;

const uint32_t SETTINGS_SCHEMA_VERSION = 2;
Setting<uint8_t, 0> activityTimeoutSeconds;
Setting<uint8_t, 1> dawnHour;
Setting<uint8_t, 2> duskHour;
Setting<uint8_t, 3> nightHour;
Setting<brightness_t, 4> daytimeBrightness;
Setting<brightness_t, 5> eveningBrightness;
Setting<brightness_t, 6> nighttimeBrightness;
Setting<OnOff, 7> lightsOn;
Setting<LowBattery, 8> lowBatteryCutoff;
Setting<OnOff, 9> sleepOn;
Setting<tint_t, 100> museumLightTint;
Setting<brightness_t, 101> museumLightBrightness;
Setting<tint_t, 200> libraryLightTint;
Setting<brightness_t, 201> libraryLightBrightness;
SettingArray<vbat_t, 1000, BATTERY_HISTORY_LENGTH> batteryHistory;
Setting<uint8_t, 2000> testSetting1;
Setting<int8_t, 2001> testSetting2;
Setting<StrandTestPattern, 2002> strandTestPattern;

void resetSettings() {
  activityTimeoutSeconds.set(30);
  dawnHour.set(7);
  duskHour.set(18);
  nightHour.set(23);
  daytimeBrightness.set(BRIGHTNESS_MAX);
  eveningBrightness.set(BRIGHTNESS_MAX);
  nighttimeBrightness.set(BRIGHTNESS_MAX);
  lightsOn.set(OnOff::ON);
  lowBatteryCutoff.set(LowBattery::V3_4);
  sleepOn.set(OnOff::ON);
  museumLightTint.set(TINT_WHITE);
  museumLightBrightness.set(BRIGHTNESS_MAX);
  libraryLightTint.set(TINT_WHITE);
  libraryLightBrightness.set(BRIGHTNESS_MAX);
  testSetting1.set(0);
  testSetting2.set(0);
  strandTestPattern.set(StrandTestPattern::DISABLED);
}

Settings settings;

Panel panel;
Binding binding(&panel);
Stage stage(&binding,
  [] { return activityTimeoutSeconds.get() * 1000UL; });

constexpr int LIGHTS_EN_PIN = 2;
constexpr int LIGHTS_PIN = 21;
constexpr unsigned LIGHTS_MUSEUM_FIRST = 0;
constexpr unsigned LIGHTS_MUSEUM_COUNT = 11;
constexpr unsigned LIGHTS_LIBRARY_FIRST = 11;
constexpr unsigned LIGHTS_LIBRARY_COUNT = 11;
constexpr unsigned LIGHTS_TOTAL_COUNT = 22;
Adafruit_NeoPixel lights(LIGHTS_TOTAL_COUNT, LIGHTS_PIN, NEO_GRBW | NEO_KHZ800);
RGBW oldLights[LIGHTS_TOTAL_COUNT];
RGBW newLights[LIGHTS_TOTAL_COUNT];
bool oldLightsEnabled;

constexpr int VBAT_PIN = A6;
constexpr int CHG_PIN = 3;
constexpr int PGOOD_PIN = 4;

SnoozeDigital snoozeDigital;
SnoozeUSBSerial snoozeUsbSerial;
SnoozeTimer snoozeTimer;
SnoozeBlock snoozeBlock(snoozeUsbSerial, snoozeDigital, snoozeTimer);

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

// Battery voltage measured in increments of 10 mV minus 3 V
// range 3.00 V to 5.55 V, assumes 12 bit analog read resolution
uint32_t batterySampleTime = 0;
vbat_t batterySample;

vbat_t readBattery() {
  uint32_t t = millis() >> 7; // don't sample any faster than about 7 Hz to avoid draining the capacitor
  if (t != batterySampleTime) {
    constexpr uint32_t vref = 331; // 3.31 V
    batterySampleTime = t;
    uint32_t voltage = analogRead(VBAT_PIN) * vref / 2047;
    batterySample = vbat_t(std::min(std::max(voltage, 300UL), 555UL) - 300UL);
  }
  return batterySample;
}

uint32_t batteryPeriod() {
   return (now() / BATTERY_HISTORY_INTERVAL) % BATTERY_HISTORY_LENGTH;
}

float batteryVoltage(vbat_t value) {
  return (value + 300) * 0.01f;
}

bool isLowBattery(LowBattery cutoff, vbat_t hysteresis) {
  switch (cutoff) {
    default:
    case LowBattery::DISABLED:
      return false;
    case LowBattery::V3_3:
      return readBattery() < 30 + hysteresis;
    case LowBattery::V3_4:
      return readBattery() < 40 + hysteresis;
    case LowBattery::V3_5:
      return readBattery() < 50 + hysteresis;
    case LowBattery::V3_6:
      return readBattery() < 60 + hysteresis;
  }
}

LowBattery currentLowBatteryCutoff;
bool currentLowBatteryState;

bool isLowBatteryWithHysteresis() {
  LowBattery setting = lowBatteryCutoff.get();
  if (currentLowBatteryCutoff != setting) {
    currentLowBatteryCutoff = setting;
    currentLowBatteryState = false;
  }

  currentLowBatteryState = isLowBattery(currentLowBatteryCutoff,
      currentLowBatteryState ? 5 : 0); // add hysteresis
  return currentLowBatteryState;
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
  menu->addItem(std::make_unique<TitleItem>("MUSEUM"));
  menu->addItem(std::make_unique<TintItem>("Light Tint", museumLightTint));
  menu->addItem(std::make_unique<BrightnessItem>("Light Brightness", museumLightBrightness));
  return menu;
}

std::unique_ptr<Menu> makeLibraryMenu() {
  auto menu = std::make_unique<Menu>();
  menu->addItem(std::make_unique<TitleItem>("LIBRARY"));
  menu->addItem(std::make_unique<TintItem>("Light Tint", libraryLightTint));
  menu->addItem(std::make_unique<BrightnessItem>("Light Brightness", libraryLightBrightness));
  return menu;
}

std::unique_ptr<Menu> makeTimeMenu() {
  auto menu = std::make_unique<Menu>();
  menu->addItem(std::make_unique<TitleItem>("TIME"));
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
  menu->addItem(std::make_unique<TitleItem>("POWER"));
  menu->addItem(std::make_unique<ChoiceItem<OnOff>>("Lights", lightsOn));
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
  menu->addItem(std::make_unique<ChoiceItem<LowBattery>>("Low Battery Cutoff", lowBatteryCutoff));
  menu->addItem(std::make_unique<ChoiceItem<OnOff>>("Sleep When Idle", sleepOn));
  return menu;
}

class BatteryMonitor : public Scene {
public:
  BatteryMonitor() {}
  virtual ~BatteryMonitor() override {}

  void poll(Context& context) override;
  void draw(Context& context, Canvas& canvas) override;
  bool input(Context& context, const InputEvent& event) override;

private:
  static constexpr uint32_t DISPLAY_WIDTH = 128;
  static constexpr uint32_t DISPLAY_HEIGHT = 64;
  static constexpr uint32_t CHART_WIDTH = 100;
  static constexpr uint32_t CHART_HEIGHT = 36;
  static constexpr uint32_t CHART_X = DISPLAY_WIDTH - CHART_WIDTH - 1;
  static constexpr uint32_t CHART_Y = DISPLAY_HEIGHT - CHART_HEIGHT - 11;
  static constexpr uint32_t SCROLL_MIN = 0;
  static constexpr uint32_t SCROLL_MAX = BATTERY_HISTORY_LENGTH - CHART_WIDTH;
  static constexpr float VOLTAGE_MIN = 3.2f;
  static constexpr float VOLTAGE_MAX = 4.2f;
  static constexpr uint32_t DIVISION_MINOR = (60 * 60) / BATTERY_HISTORY_INTERVAL; // every hour
  static constexpr uint32_t DIVISION_MAJOR = DIVISION_MINOR * 12;

  enum class State {
    DISCHARGING, CHARGING, POWERED
  };

  time_t _time = 0;
  vbat_t _level = 0;
  uint32_t _scroll = SCROLL_MAX;
  State _state = State::DISCHARGING;
};

void BatteryMonitor::poll(Context& context) {
  time_t t = now();
  if (t != _time) {
    _time = t;
    _level = readBattery();
    context.requestDraw();
  }

  State newState = State::DISCHARGING;
  if (!digitalRead(CHG_PIN)) {
    newState = State::CHARGING;
  } else if (!digitalRead(PGOOD_PIN)) {
    newState = State::POWERED;
  }
  if (newState != _state) {
    _state = newState;
    context.requestDraw();
  }
}

void BatteryMonitor::draw(Context& context, Canvas& canvas) {
  canvas.gfx().setCursor(1, 0);
  canvas.gfx().setFont(TITLE_FONT);
  canvas.gfx().print("BATTERY: ");
  canvas.gfx().print(batteryVoltage(_level), 2);
  canvas.gfx().print(" V ");

  canvas.gfx().setFont(u8g2_font_open_iconic_embedded_1x_t);
  switch (_state) {
    case State::CHARGING:
      canvas.gfx().drawStr(109, 1, "C");
      break;
    case State::POWERED:
      canvas.gfx().drawStr(109, 1, "B");
      break;
    default:
      break;
  }
  canvas.gfx().drawStr(118, 1, _level < 50 ? "@" : "I");

  // Draw the Y axis and labels
  canvas.gfx().setFont(u8g2_font_4x6_tr);
  canvas.gfx().drawLine(CHART_X - 1, CHART_Y, CHART_X - 1, CHART_Y + CHART_HEIGHT - 1);
  canvas.gfx().drawStr(2, CHART_Y - 3, "4.2 V");
  canvas.gfx().drawLine(CHART_X - 4, CHART_Y, CHART_X - 2, CHART_Y);
  uint32_t v37elev = (3.7 - VOLTAGE_MIN) * CHART_HEIGHT / (VOLTAGE_MAX - VOLTAGE_MIN);
  canvas.gfx().drawStr(2, CHART_Y + CHART_HEIGHT - 1 - v37elev - 3, "3.7 V");
  canvas.gfx().drawLine(CHART_X - 4, CHART_Y + CHART_HEIGHT - 1 - v37elev, CHART_X - 2, CHART_Y + CHART_HEIGHT - 1 - v37elev);
  canvas.gfx().drawStr(2, CHART_Y + CHART_HEIGHT - 1 - 3, "3.2 V");
  canvas.gfx().drawLine(CHART_X - 4, CHART_Y + CHART_HEIGHT - 1, CHART_X - 2, CHART_Y + CHART_HEIGHT - 1);

  // Draw the chart and X axis labels
  uint32_t currentPeriod = batteryPeriod();
  for (uint32_t pos = 0; pos < CHART_WIDTH; pos++) {
    uint32_t index = pos + _scroll;
    uint32_t period = (index + currentPeriod + 1) % BATTERY_HISTORY_LENGTH;
    float voltage = std::min(std::max(batteryVoltage(batteryHistory.getAt(period)), VOLTAGE_MIN), VOLTAGE_MAX);
    uint32_t elev = (voltage - VOLTAGE_MIN) * CHART_HEIGHT / (VOLTAGE_MAX - VOLTAGE_MIN);
    uint32_t x = pos + CHART_X;
    canvas.gfx().drawBox(x, CHART_Y + CHART_HEIGHT - 1 - elev, 1, elev + 1);

    uint32_t age = BATTERY_HISTORY_LENGTH - index - 1;
    if ((age % DIVISION_MAJOR) == 0) {
      canvas.gfx().drawLine(x, CHART_Y + CHART_HEIGHT, x, CHART_Y + CHART_HEIGHT + 3);
      String text;
      text.append(age / DIVISION_MINOR);
      text.append('h');
      canvas.gfx().drawStr(x - canvas.gfx().getStrWidth(text.c_str()) + 1, CHART_Y + CHART_HEIGHT + 4, text.c_str());
    } else if ((age % DIVISION_MINOR) == 0) {
      canvas.gfx().drawLine(x, CHART_Y + CHART_HEIGHT, x, CHART_Y + CHART_HEIGHT + 1);
    }
  }
}

bool BatteryMonitor::input(Context& context, const InputEvent& event) {
  switch (event.type) {
    case InputType::ROTATE:
      _scroll = std::min(std::max(int32_t(_scroll) + event.value * 4, int32_t(SCROLL_MIN)), int32_t(SCROLL_MAX));
      context.requestDraw();
      return true;
    default:
      return false;
  }
}

std::unique_ptr<BatteryMonitor> makeBatteryMonitorScene() {
  return std::make_unique<BatteryMonitor>();
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
  time_t _time = 0;
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
  canvas.gfx().setFont(TITLE_FONT);
  canvas.gfx().drawStr(1, 0, "BOARD TEST");
  canvas.gfx().setFont(DEFAULT_FONT);

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
  menu->addItem(std::make_unique<TitleItem>("EEPROM TEST"));
  menu->addItem(std::make_unique<NumericItem<uint8_t>>("Test Setting 1",
    testSetting1, 0, 100, 5));
  menu->addItem(std::make_unique<NumericItem<int8_t>>("Test Setting 2",
    testSetting2, -10, 10, 1));
  return menu;
}

std::unique_ptr<Menu> makeStrandTestMenu() {
  auto menu = std::make_unique<Menu>();
  menu->addItem(std::make_unique<TitleItem>("STRAND TEST"));
  menu->addItem(std::make_unique<ChoiceItem<StrandTestPattern>>("Pattern", strandTestPattern));
  return menu;
}

std::unique_ptr<Menu> makeFactoryResetMenu() {
  auto menu = std::make_unique<Menu>();
  menu->addItem(std::make_unique<TitleItem>("FACTORY RESET"));
  menu->addItem(std::make_unique<BackItem>("Back Away Slowly..."));
  menu->addItem(std::make_unique<NavigateItem>("Erase All Settings!", Settings::eraseAndReboot));
  return menu;
}

std::unique_ptr<Menu> makeDiagnosticsMenu() {
  auto menu = std::make_unique<Menu>();
  menu->addItem(std::make_unique<TitleItem>("DIAGNOSTICS"));
  menu->addItem(std::make_unique<NavigateItem>("Board Test", makeBoardTestScene));
  menu->addItem(std::make_unique<NavigateItem>("EEPROM Test", makeEepromTestMenu));
  menu->addItem(std::make_unique<NavigateItem>("Strand Test", makeStrandTestMenu));
  menu->addItem(std::make_unique<NavigateItem>("Factory Reset", makeFactoryResetMenu));
  return menu;
}

std::unique_ptr<Menu> makeRootMenu() {
  auto menu = std::make_unique<Menu>();
  menu->addItem(std::make_unique<TitleItem>("LITTLE FREE TOWN"));
  menu->addItem(std::make_unique<NavigateItem>("Museum", makeMuseumMenu));
  menu->addItem(std::make_unique<NavigateItem>("Library", makeLibraryMenu));
  menu->addItem(std::make_unique<NavigateItem>("Time", makeTimeMenu));
  menu->addItem(std::make_unique<NavigateItem>("Power Saving", makePowerSavingMenu));
  menu->addItem(std::make_unique<NavigateItem>("Battery Monitor", makeBatteryMonitorScene));
  menu->addItem(std::make_unique<NavigateItem>("Diagnostics", makeDiagnosticsMenu));
  return menu;
}

LightState renderMuseumLights(float scale) {
  RGBW color = makeStripColor(museumLightTint.get(), museumLightBrightness.get()) * scale;
  for (size_t i = 0; i < LIGHTS_MUSEUM_COUNT; i++) {
    newLights[i + LIGHTS_MUSEUM_FIRST] = color;
  }
  return LightState::ON;
}

LightState renderLibraryLights(float scale) {
  RGBW color = makeStripColor(libraryLightTint.get(), libraryLightBrightness.get()) * scale;
  for (size_t i = 0; i < LIGHTS_LIBRARY_COUNT; i++) {
    newLights[i + LIGHTS_LIBRARY_FIRST] = color;
  }
  return LightState::ON;
}

LightState renderStrandTest() {
  switch (strandTestPattern.get()) {
    default:
    case StrandTestPattern::DISABLED: {
      return LightState::OFF;
    }
    case StrandTestPattern::WHITE: {
      for (size_t i = 0; i < LIGHTS_TOTAL_COUNT; i++) {
        newLights[i] = RGBW{0, 0, 0, 255};
      }
      return LightState::ON;
    }
    case StrandTestPattern::GLOW: {
      uint8_t pos = millis() >> 4;
      for (size_t i = 0; i < LIGHTS_TOTAL_COUNT; i++) {
        newLights[i] = RGBW{0, 0, 0, uint8_t(pos + i)};
      }
      return LightState::ANIMATING;
    }
    case StrandTestPattern::RAINBOW: {
      uint8_t pos = millis() >> 4;
      for (size_t i = 0; i < LIGHTS_TOTAL_COUNT; i++) {
        newLights[i] = colorWheel(pos + i).toRGBW();
      }
      return LightState::ANIMATING;
    }
  }  
}

LightState renderLights() {
  if (isLowBatteryWithHysteresis()) return LightState::OFF;

  for (size_t i = 0; i < LIGHTS_TOTAL_COUNT; i++) {
    newLights[i] = RGBW{};
  }

  LightState state = renderStrandTest();
  if (state == LightState::OFF && lightsOn.get() != OnOff::OFF) {
    const float scale = brightnessForTimeOfDay() * 0.1f;
    state = renderMuseumLights(scale);
    state = mergeLightState(state, renderLibraryLights(scale));
  }
  return state;
}

void setLightsEnabled(bool enabled) {
  if (enabled) {
    // Turn on the mosfet that drives LED GND and set the LED data pin
    // to output mode.
    digitalWrite(LIGHTS_EN_PIN, HIGH);
    delay(1);
    lights.begin();
  } else {
    // Turn off the mosfet that drives LED GND and also set the LED data
    // pin to a high impedance state to prevent vampire current draw from
    // the LEDs via the data pin while LED GND is floating.
    pinMode(LIGHTS_PIN, INPUT);
    digitalWrite(LIGHTS_EN_PIN, LOW);
  }
}

LightState updateLights() {
  bool changed = false;

  LightState state = renderLights();
  bool lightsEnabled = state != LightState::OFF;
  if (lightsEnabled != oldLightsEnabled) {
    oldLightsEnabled = lightsEnabled;
    changed = true;
    setLightsEnabled(lightsEnabled);
  }

  if (lightsEnabled) {
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
  return state;
}

void updateBatteryHistory() {
  static uint32_t lastPeriod = 0;
  uint32_t period = batteryPeriod();
  if (period != lastPeriod) {
    vbat_t level = readBattery();
    lastPeriod = period;
    batteryHistory.setAt(period, level);

    Serial.print("Battery: ");
    Serial.print(batteryVoltage(level), 2);
    Serial.print(" V at ");
    printDateAndTime(Serial, now());
    Serial.println();
  }
}

void setup() {
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
  panel.begin(snoozeDigital);
  stage.begin(makeRootMenu());
  pinMode(LIGHTS_EN_PIN, OUTPUT);
  setLightsEnabled(false);

  // Initialize battery monitor
  pinMode(CHG_PIN, INPUT);
  pinMode(PGOOD_PIN, INPUT);
  analogReadResolution(12);

  // Setup sleeping
  // Periodically wake to update battery stats
  snoozeTimer.setTimer(60000);
#if USE_BUILTIN_LED
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);
#endif
}

void loop() {
  updateBatteryHistory();
  panel.update();
  stage.update();
  LightState state = updateLights();

  static uint32_t readyToSleepAt = 0;
  if (sleepOn.get() == OnOff::ON && state != LightState::ANIMATING
      && panel.canSleep() && stage.canSleep()) {
    uint32_t time = millis();
    if (!readyToSleepAt) {
      readyToSleepAt = time;
    } else if (time - readyToSleepAt >= 500) {
#if USE_BUILTIN_LED
      digitalWrite(LED_BUILTIN, LOW);
#endif
      // Go to sleep.  We can't use deepSleep() because not all of the inputs
      // we need to monitor support low-level wakeups (see LLWU matrix in processor
      // documentation).
      Snooze.sleep(snoozeBlock);
      readyToSleepAt = 0;
    }
  } else {
    readyToSleepAt = 0;
  }
#if USE_BUILTIN_LED
  digitalWrite(LED_BUILTIN, HIGH);
#endif
}
