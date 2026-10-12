// Pro Micro (3.3V, ATmega32U4) weather interface: reads the Argent Data wind and rain sensors and serves
// wind speed, direction and rainfall to a RAK WisBlock as an I2C slave. See README.md for wiring and the
// register map.
//
// This file only brings up the hardware and moves each second's readings into the WeatherStation; the
// hardware drivers are the other files in src/, and the averaging lives in lib/WeatherCore.

#include <Arduino.h>

#include "Board.h"
#include "DisableUnusedHardware.h"
#include "ReedSwitches.h"
#include "IICServer.h"
#include "OneHzSleep.h"
#include "VaneAdc.h"
#include "Watchdog.h"
#include "WeatherStation.h"

static WeatherStation station;

void setup() {
#ifdef WEATHER_DEBUG
  BOARD_DEBUG_SERIAL.begin(WEATHER_DEBUG_BAUD);
  BOARD_DEBUG_SERIAL.println(F("weather interface"));
#endif

  DisableUnusedHardware::begin();
  OneHzSleep::begin();
  ReedSwitches::begin();
  VaneAdc::begin();
  IICServer::begin();
  IICServer::update(station.currentRegisters());
  Watchdog::begin();
}

void loop() {
  const uint8_t actual_elapsed_seconds = OneHzSleep::sleepOneSecond();
  Watchdog::rearm();

  const ReedSwitches::Counts counts = ReedSwitches::take();
  const WeatherSample sample = {
    actual_elapsed_seconds,
    counts.wind_pulses,
    counts.rain_tips,
    VaneAdc::sample()
  };
  station.add(sample);

  WeatherRegisters registers = station.currentRegisters();
  IICServer::update(registers);

#ifdef WEATHER_DEBUG
  BOARD_DEBUG_SERIAL.print(F("pulses="));
  BOARD_DEBUG_SERIAL.print(sample.wind_pulses);
  BOARD_DEBUG_SERIAL.print(F(" vane_raw="));
  BOARD_DEBUG_SERIAL.print(sample.vane_10bit_adc);
  BOARD_DEBUG_SERIAL.print(F(" avg="));
  BOARD_DEBUG_SERIAL.print(registers.wind_avg);
  BOARD_DEBUG_SERIAL.print(F(" gust="));
  BOARD_DEBUG_SERIAL.print(registers.wind_gust);
  BOARD_DEBUG_SERIAL.print(F(" dir="));
  BOARD_DEBUG_SERIAL.print(registers.wind_direction);
  BOARD_DEBUG_SERIAL.print(F(" rain_1h="));
  BOARD_DEBUG_SERIAL.println(registers.rain_last_hour);
  BOARD_DEBUG_SERIAL.flush();
#endif
}
