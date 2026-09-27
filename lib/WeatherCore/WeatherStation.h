#pragma once

#include <stdint.h>

#include "RainGauge.h"
#include "WeatherRegisters.h"
#include "WindSensor.h"

// The wind sensor and rain gauge together, fed once per second from the hardware, producing the register
// map the RAK reads.
class WeatherStation {
public:
  // Flag that the last reset was the watchdog, for the status register.
  void setWatchdogReset(bool watchdog_reset) { _watchdog_reset = watchdog_reset; }

  // Record `elapsed_seconds` of time (1 unless the caller fell behind) with the readings taken over it:
  // anemometer pulses, the vane's ADC reading, and rain bucket tips. A backlog lands in one second of data
  // so no pulses or tips are lost.
  void addSeconds(uint8_t elapsed_seconds, uint8_t wind_pulses, uint16_t vane_counts, uint8_t rain_tips);

  // The current values, as served over I2C.
  WeatherRegisters registers() const;

  const WindSensor& wind() const { return _wind; }
  const RainGauge& rain() const { return _rain; }

private:
  WindSensor _wind;
  RainGauge _rain;
  uint32_t _uptime_seconds = 0;
  uint8_t _sequence = 0;
  bool _watchdog_reset = false;
};
