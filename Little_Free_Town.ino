#include <memory>

#include <TimeLib.h>
#include <Adafruit_SleepyDog.h>

#include "panel.h"
#include "settings.h"
#include "ui.h"

Settings settings;
Panel panel;
Binding binding(&panel);
Stage stage(&binding);

const uint32_t SETTINGS_SCHEMA_VERSION = 1;
Setting<bool, 0> alwaysOn;

void printTime() {
  TimeElements time;
  breakTime(now(), time);
  Serial.print("Time: ");
  Serial.print(time.Year + 1970);
  Serial.print('/');
  Serial.print(time.Month);
  Serial.print('/');
  Serial.print(time.Day);
  Serial.print(' ');
  Serial.print(time.Hour);
  Serial.print(':');
  if (time.Minute < 10) Serial.print('0');
  Serial.print(time.Minute);
  Serial.print(':');
  if (time.Second < 10) Serial.print('0');
  Serial.print(time.Second);
  Serial.println(); 
}

void printWelcome() {
  Serial.println("Little Free Town\n");
  if (timeStatus() == timeSet) {
    Serial.println("Time was synchronized with the host PC");
  }
  printTime();
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
