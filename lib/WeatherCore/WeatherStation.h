#pragma once

#include <stdint.h>

#include "WeatherRegisters.h"

// Everything the board measures in one second. The board fills one in each second and passes it to
// WeatherStation::add(). Count the reed switches with a PulseCounter each.
struct WeatherSample {
  // Seconds since the previous sample, timed from an accurate clock: wind speed is pulses per second, so
  // timing error is speed error. 1, unless the board fell behind.
  uint8_t seconds;
  uint8_t wind_pulses;   // anemometer closures since the previous sample
  uint8_t rain_tips;     // rain gauge closures since the previous sample
  uint16_t vane_10bit_adc;   // one 10-bit vane reading (VaneDecoder), taken at any point in the second
};

// The wind sensor and rain gauge together, fed once per second from the hardware, producing the
// WeatherRegisters values. It is not interrupt-safe: count pulses in interrupts with PulseCounter, and call
// add() and currentRegisters() from the main loop.
// There is one station, so its state lives in WeatherStation.cpp.
class WeatherStation {
public:
  WeatherStation() = delete;

  // Record one sample. A backlog (seconds > 1) advances uptime by all of it but lands in one second of
  // wind and rain data, so no pulses or tips are lost.
  static void add(const WeatherSample& sample);

  static WeatherRegisters currentRegisters();

  // Back to the power-up state, for tests. Builds the replacement sensors on the stack, so keep it off
  // the AVR.
  static void reset();
};
