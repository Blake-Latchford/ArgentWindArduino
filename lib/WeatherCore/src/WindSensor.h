#pragma once

#include <stdint.h>

#include "VaneDecoder.h"

// The Argent Data anemometer and wind vane, which share one cable. Takes one sample per second (the
// anemometer's pulse count and the vane's decoded heading) and keeps them in ring buffers over the reporting
// window. Gives out wind speed and direction. Speed and direction live in one sensor because the
// direction average is weighted by speed.
//
// Speeds are in 0.01 m/s. The anemometer closes once per second at 2.4 km/h, so one pulse per second is
// 2/3 m/s. Directions are in 1/65536 of a turn clockwise from north, so they wrap past north like a
// uint16_t: 0x4000 is east, 0x8000 south, 0xC000 west.

#ifndef WIND_WINDOW_SECONDS
#define WIND_WINDOW_SECONDS 600U  // 10 minutes, the WMO sustained-wind window
#endif

class WindSensor {
public:
  // One second of readings.
  struct WindSample {
    uint8_t pulses;    // anemometer closures during the second
    Heading heading;   // the vane's heading (see VaneDecoder), or Heading::Invalid for a vane fault
  };

  // Record one second's sample.
  void addSample(WindSample sample);

  // Speed and direction are only calculated over a full window. Until then they read 0 and invalid.
  bool windowFull() const { return _filled == WIND_WINDOW_SECONDS; }

  // Mean speed over the window, or 0 until the window is full.
  uint16_t averageSpeed() const;

  // Highest rolling WIND_GUST_SECONDS mean in the window, or 0 until the window is full.
  uint16_t gustSpeed() const;

  struct Direction {
    // False until the window is full, in a calm, when there is no valid heading in the window, or when
    // the vectors cancel exactly.
    bool valid = false;
    uint16_t angle = 0;   // speed-weighted vector mean of the vane headings
    // Standard deviation of the headings by the Yamartino method, from unit vectors (every second with a
    // valid heading counts the same, whatever its speed). Reported in a calm too, but 0 until the window is
    // full or when there is no valid heading in it.
    uint16_t stddev = 0;
  };

  Direction direction() const;

private:
  WindSample _samples[WIND_WINDOW_SECONDS];
  uint16_t _head = 0;     // next slot to write
  uint16_t _filled = 0;

  // Running sums over the window, so each second's update is O(1).
  uint32_t _pulse_sum = 0;
  int64_t _weighted_sin = 0, _weighted_cos = 0;   // up to 600 s x 255 pulses x 32767, past int32_t
  uint16_t _heading_count = 0;       // seconds with a valid heading
  int32_t _unit_sin = 0, _unit_cos = 0;

  void addToSums(const WindSample& sample) { changeSums(sample, 1); }
  void removeFromSums(const WindSample& sample) { changeSums(sample, -1); }
  void changeSums(const WindSample& sample, int8_t sign);
  uint16_t stddev() const;
};
