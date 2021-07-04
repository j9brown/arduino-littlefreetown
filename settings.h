/*
 * Stores typed values in EEPROM with a checksum.
 */

#pragma once

#include <Arduino.h>
#include <EEPROM.h>

// Accessor for a setting stored in EEPROM.
// Settings must not overlap in memory.
template<typename T, unsigned addr>
class Setting {
public:
  T get() const {
    T value;
    uint8_t* bytes = reinterpret_cast<uint8_t*>(&value);
    for (size_t i = 0; i < sizeof(T); i++)
      bytes[i] = EEPROM[4 + addr + i];
    return value;
  }

  void set(T value) const {
    const uint8_t* bytes = reinterpret_cast<uint8_t*>(&value);
    for (size_t i = 0; i < sizeof(T); i++)
      EEPROM[4 + addr + i].update(bytes[i]);
  }
};

// Initializes the EEPROM for settings.
// Erases all settings if the schema has changed.
class Settings {
public:
  Settings() {}

  void begin(uint8_t schemaVersion) {
    Setting<uint32_t, 0> magic;
    const uint32_t expected = 0xAB5C155A ^ schemaVersion;
    if (magic.get() != expected) {
      clear();
      magic.set(expected);
    }
  }

private:
  Settings(const Settings&) = delete;
  Settings(Settings&&) = delete;  
  Settings& operator=(const Settings&) = delete;
  Settings& operator=(Settings&&) = delete;

  void clear() {
    for (size_t i = 4; i < EEPROM.length(); i++) {
      EEPROM[i].update(0);
    }
  }
};
