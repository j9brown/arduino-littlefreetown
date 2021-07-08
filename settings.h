/*
 * Stores typed values in EEPROM with a checksum.
 */

#pragma once

#include <Arduino.h>
#include <EEPROM.h>

// Initializes the EEPROM for settings.
// Erases all settings if the schema has changed.
class Settings {
public:
  Settings() {}
  ~Settings() = default;

  using InitCallback = void (*)();

  static void begin(uint8_t schemaVersion, InitCallback init) {
    const uint32_t expected = 0xAB5C155A ^ schemaVersion;
    if (read<uint32_t>(EEPROM.length() - 4) != expected) {
      clear();
      init();
      write<uint32_t>(EEPROM.length() - 4, expected);
    }
  }

  static void eraseAndReboot() {
    write<uint32_t>(EEPROM.length() - 4, 0);
    _reboot_Teensyduino_();
  }

  template <typename T>
  static T read(unsigned addr) {
    T value;
    uint8_t* bytes = reinterpret_cast<uint8_t*>(&value);
    for (size_t i = 0; i < sizeof(T); i++)
      bytes[i] = EEPROM[addr + i];
    return value;
  }

  template <typename T>
  static void write(unsigned addr, T value) {
    const uint8_t* bytes = reinterpret_cast<uint8_t*>(&value);
    for (size_t i = 0; i < sizeof(T); i++)
      EEPROM[addr + i].update(bytes[i]);
  }

private:
  Settings(const Settings&) = delete;
  Settings(Settings&&) = delete;  
  Settings& operator=(const Settings&) = delete;
  Settings& operator=(Settings&&) = delete;

  static void clear() {
    for (size_t i = 0; i < size_t(EEPROM.length()); i++) {
      EEPROM[i].update(0);
    }
  }
};

// Accessor for a setting stored in EEPROM.
// Settings must not overlap in memory.
template<typename T, unsigned addr>
class Setting {
public:
  static T get() {
    return Settings::read<T>(addr);
  }

  static void set(T value) {
    return Settings::write<T>(addr, std::move(value));
  }
};
