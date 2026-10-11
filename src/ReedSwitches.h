#pragma once

#include <stdint.h>

// Anemometer and rain gauge reed switches (pins in Board.h), counted on falling edges by pin-change
// interrupt. Each pin has an external 100k pull-up and an RC debounce network.
class ReedSwitches {
 public:
  ReedSwitches() = delete;

  static void begin();

  struct Counts {
    uint8_t wind_pulses;
    uint8_t rain_tips;
  };

  // Counts since the last call. Correct up to 255 per call; beyond that they wrap.
  static Counts take();
};
