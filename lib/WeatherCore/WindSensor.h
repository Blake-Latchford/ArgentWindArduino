#pragma once

#include <stdint.h>

#include "VaneDecoder.h"

// The Argent Data anemometer and wind vane, which share one cable. Takes one sample per second (the
// anemometer's pulse count and the vane's ADC reading) and keeps them in ring buffers over the reporting
// window. Gives out wind speed and direction. Speed and direction live in one sensor because the
// direction average is weighted by speed.
//
// Speeds are in 0.01 m/s. The anemometer closes once per second at 2.4 km/h, so one pulse per second is
// 2/3 m/s. Directions are in tenths of a degree clockwise from north.

#ifndef WIND_WINDOW_SECONDS
#define WIND_WINDOW_SECONDS 600   // 10 minutes, the WMO sustained-wind window
#endif

#define WIND_GUST_SECONDS 3       // WMO gust: highest rolling 3-second mean

class WindSensor {
public:
  // Record one second: `pulses` anemometer closures during it, and the vane's 10-bit ADC reading.
  void addSecond(uint8_t pulses, uint16_t vane_counts);

  // Speed and direction are only calculated over a full window. Until then they read 0 and invalid.
  bool windowFull() const { return _filled == WIND_WINDOW_SECONDS; }

  // Mean speed over the window, or 0 until the window is full.
  uint16_t averageSpeed() const;

  // Highest rolling WIND_GUST_SECONDS mean in the window, or 0 until the window is full.
  uint16_t gustSpeed() const;

  // Speed-weighted vector mean of the vane headings, 0-3599. Returns false, leaving `tenths` alone, until
  // the window is full, in a dead calm (no pulses in the window), when there is no valid heading in the
  // window, or when the vectors cancel exactly.
  bool direction(uint16_t& tenths) const;

  // The latest vane reading was an open circuit or a short.
  bool vaneFault() const { return _vane_fault; }

private:
  struct Second {
    uint8_t pulses;
    Heading heading;   // or Heading::Invalid
  };

  Second _seconds[WIND_WINDOW_SECONDS];
  uint16_t _head = 0;     // next slot to write
  uint16_t _filled = 0;
  bool _vane_fault = false;

  // Running sums over the window, so each second's update is O(1).
  uint32_t _pulse_sum = 0;
  int32_t _weighted_sin = 0, _weighted_cos = 0;

  void addToSums(const Second& second) { changeSums(second, 1); }
  void removeFromSums(const Second& second) { changeSums(second, -1); }
  void changeSums(const Second& second, int8_t sign);
};
