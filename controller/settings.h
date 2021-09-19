/*
 * Stores typed values in EEPROM with a checksum.
 */

#pragma once

#include <Arduino.h>
#include <EEPROM.h>

using eeprom_addr_t = uint16_t;

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
      clear(0, EEPROM.length());
      init();
      write<uint32_t>(EEPROM.length() - 4, expected);
    }
  }

  static void eraseAndReboot() {
    write<uint32_t>(EEPROM.length() - 4, 0);
    _reboot_Teensyduino_();
  }

  template <typename T>
  static T read(eeprom_addr_t addr) {
    T value;
    uint8_t* bytes = reinterpret_cast<uint8_t*>(&value);
    for (size_t i = 0; i < sizeof(T); i++)
      bytes[i] = EEPROM[addr + i];
    return value;
  }

  template <typename T>
  static void write(eeprom_addr_t addr, T value) {
    const uint8_t* bytes = reinterpret_cast<uint8_t*>(&value);
    for (size_t i = 0; i < sizeof(T); i++)
      EEPROM[addr + i].update(bytes[i]);
  }

  static void clear(eeprom_addr_t addr, size_t length) {
    for (size_t i = 0; i < length; i++) {
      EEPROM[addr + i].update(0);
    }
  }

private:
  Settings(const Settings&) = delete;
  Settings(Settings&&) = delete;  
  Settings& operator=(const Settings&) = delete;
  Settings& operator=(Settings&&) = delete;
};

// Accessor for a setting stored in EEPROM.
// Settings must not overlap in memory.
template<typename T>
class Setting {
public:
  explicit Setting(eeprom_addr_t addr) : _addr(addr) {}

  T get() const {
    return Settings::read<T>(_addr);
  }

  void set(T value) const {
    return Settings::write<T>(_addr, std::move(value));
  }

private:
  const eeprom_addr_t _addr;
};

// Accessor for an array of settings stored in EEPROM.
// Settings must not overlap in memory.
template<typename T, size_t count>
class SettingArray {
public:
  explicit SettingArray(eeprom_addr_t addr) : _addr(addr) {}

  T getAt(size_t i) const {
    return Settings::read<T>(_addr + sizeof(T) * i);
  }

  void setAt(size_t i, T value) const {
    return Settings::write<T>(_addr + sizeof(T) * i, std::move(value));
  }

  void clear() const {
    Settings::clear(_addr, sizeof(T) * count);
  }

private:
  const eeprom_addr_t _addr;
};
