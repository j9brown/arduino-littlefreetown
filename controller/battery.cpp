#include <algorithm>

#include <TimeLib.h>

#include "battery.h"
#include "utils.h"

namespace {
// Remember when the battery history was last updated using a location in NVRAM
// to minimize wear on the flash memory.
constexpr volatile uint32_t* LAST_SAMPLE_INDEX = reinterpret_cast<volatile uint32_t*>(0x4003E000);
}

Battery::Battery(int pin) : _pin(pin) {}

void Battery::begin() {
  analogReadResolution(12);
}

millivolt_t Battery::read() {
  uint32_t t = millis() >> 7; // don't sample any faster than about 7 Hz to avoid draining the capacitor
  if (t != _sampleTime) {
    constexpr uint32_t vref = 3310; // 3.310 V
    _sampleTime = t;
    _sample = millivolt_t(analogRead(_pin) * vref / 2047);
  }
  return _sample;
}

BatteryHistory::BatteryHistory(Battery* battery, Storage storage) :
    _battery(battery), _storage(storage) {}

uint32_t BatteryHistory::currentPeriod() {
  return (now() / INTERVAL) % LENGTH;
}

void BatteryHistory::begin() {
  uint32_t index = now() / INTERVAL;
  uint32_t next = *LAST_SAMPLE_INDEX + 1;
  uint32_t cleared = 0;
  while (next < index && cleared < LENGTH) {
    _storage.setAt(next % LENGTH, 0);
    next++;
    cleared++;
  }

  writeSample(index);
}

void BatteryHistory::update() {
  uint32_t index = now() / INTERVAL;
  if (*LAST_SAMPLE_INDEX == index) return;

  writeSample(index);
}

void BatteryHistory::writeSample(uint32_t index) {  
  millivolt_t voltage = _battery->read();

  uint8_t level = voltage <= 3000 ? 0 : voltage >= 5550 ? 255 : uint8_t((voltage - 3000) / 10);
  _storage.setAt(index % LENGTH, level);

  *LAST_SAMPLE_INDEX = index;

#if 0
  Serial.print("Battery: ");
  Serial.print(voltage * 0.001f, 2);
  Serial.print(" V at ");
  printDateAndTime(Serial, now());
  Serial.println();
#endif
}

millivolt_t BatteryHistory::getAt(uint32_t period) const {
  return _storage.getAt(period) * 10U + 3000;
}

LowBatteryDetector::LowBatteryDetector(Battery* battery,
    GetThresholdCallback getThresholdCallback) :
    _battery(battery), _getThresholdCallback(std::move(getThresholdCallback)) {}

bool LowBatteryDetector::isLowBattery() {
  millivolt_t threshold = _getThresholdCallback();
  if (threshold != _threshold) {
    _threshold = threshold;
    _low = false;
  }

  millivolt_t hysteresis = _low ? 500 : 0;
  _low = _battery->read() < _threshold + hysteresis;
  return _low;
}
