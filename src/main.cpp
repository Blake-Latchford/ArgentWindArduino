// Pro Mini (3.3V, ATmega328P) weather interface: reads the Argent Data wind and rain sensors and serves
// wind speed, direction and rainfall to a RAK WisBlock as an I2C slave. See README.md for wiring and the
// register map.
//
// This file only brings up the hardware and moves each second's readings into the WeatherStation; the
// hardware drivers are the other files in src/, and the averaging lives in lib/WeatherCore.

#include <Arduino.h>

#include "DisableUnusedHardware.h"
#include "ReedSwitches.h"
#include "IICServer.h"
#include "OneHzSleep.h"
#include "VaneAdc.h"
#include "Watchdog.h"
#include "WeatherStation.h"

#ifndef WEATHER_I2C_ADDRESS
#define WEATHER_I2C_ADDRESS WEATHER_I2C_PRIMARY_ADDRESS
#endif

static WeatherStation station;

#ifdef WEATHER_DEBUG
static void printDebug(const ReedSwitches::Counts& counts, uint16_t vane_counts, const WeatherRegisters& r);
#endif

void setup() {
  station.setWatchdogReset(Watchdog::causedLastReset());

#ifdef WEATHER_DEBUG
  Serial.begin(WEATHER_DEBUG_BAUD);
  Serial.println(F("weather interface"));
#endif

  DisableUnusedHardware::begin();
  ReedSwitches::begin();
  VaneAdc::begin();
  IICServer::begin(WEATHER_I2C_ADDRESS);
  IICServer::update(station.registers());
  OneHzSleep::begin();
  Watchdog::begin();
}

void loop() {
  uint8_t seconds = OneHzSleep::untilNextSecond();
  Watchdog::rearm();

  ReedSwitches::Counts counts = ReedSwitches::take();
  uint16_t vane_counts = VaneAdc::sample();
  station.addSeconds(seconds, counts.wind_pulses, vane_counts, counts.rain_tips);

  WeatherRegisters registers = station.registers();
  IICServer::update(registers);

#ifdef WEATHER_DEBUG
  printDebug(counts, vane_counts, registers);
#endif
}

#ifdef WEATHER_DEBUG
// The raw vane reading is printed for calibrating the vane table (see README.md).
static void printDebug(const ReedSwitches::Counts& counts, uint16_t vane_counts, const WeatherRegisters& r) {
  Serial.print(F("pulses="));
  Serial.print(counts.wind_pulses);
  Serial.print(F(" vane_raw="));
  Serial.print(vane_counts);
  Serial.print(F(" avg="));
  Serial.print(r.wind_avg);
  Serial.print(F(" gust="));
  Serial.print(r.wind_gust);
  Serial.print(F(" dir="));
  Serial.print(r.wind_direction);
  Serial.print(F(" rain_1h="));
  Serial.println(r.rain_last_hour);
  Serial.flush();
}
#endif
