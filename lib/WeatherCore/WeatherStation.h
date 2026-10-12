#pragma once

#include <stdint.h>

#include "WeatherRegisters.h"

// One second of readings.
struct WeatherSample {
  uint8_t wind_pulses;   // anemometer closures since the previous sample
  uint8_t rain_tips;     // rain gauge closures since the previous sample
  uint16_t vane_10bit_adc;   // one 10-bit vane reading (VaneDecoder), taken at any point in the second
};

// 
class WeatherStation {
public:
  WeatherStation() = delete;

  // Assumed to be invoked once per second.
  // Calling at a different rate will skew the time scale of measurements.
  static void add(const WeatherSample& sample);

  static WeatherRegisters currentRegisters();

  // Back to the power-up state, for unit tests.
  static void reset();
};