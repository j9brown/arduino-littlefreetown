/*
 * Battery voltage monitoring.
 */

#pragma once

#include <Arduino.h>

#include "settings.h"

using millivolt_t = uint16_t;

// Reads the battery voltage from an analog pin with a voltage divider.
//
// Wiring: 100 K resistor to Vbat
//         100 K resistor to GND
//         100 nF capacitor to GND
class Battery {
public:
  explicit Battery(int pin);

  void begin();
  millivolt_t read();

private:
  const int _pin;

  uint32_t _sampleTime = 0xffffffff;
  millivolt_t _sample = 0;
};

// Maintains a record of recent battery voltage levels.
class BatteryHistory {
public:
  constexpr static time_t INTERVAL = 60 * 15; // sample every 15 minutes
  constexpr static unsigned LENGTH = 256;

  using Storage = SettingArray<uint8_t, LENGTH>;

  BatteryHistory(Battery* battery, Storage storage);

  static uint32_t currentPeriod();

  void begin();
  void update();

  millivolt_t getAt(uint32_t period) const;

private:
  Battery* const _battery;
  Storage const _storage;

  void writeSample(uint32_t index);
};

class LowBatteryDetector {
public:
  using GetThresholdCallback = millivolt_t (*)();

  LowBatteryDetector(Battery* battery, GetThresholdCallback getThresholdCallback);

  bool isLowBattery();

private:
  Battery* const _battery;
  GetThresholdCallback const _getThresholdCallback;

  millivolt_t _threshold = 0;
  bool _low = false;
};
