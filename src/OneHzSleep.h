#pragma once

#include <stdint.h>

// 1Hz time base from Timer1.
class OneHzSleep {
 public:
  OneHzSleep() = delete;

  // Timer1 counts per second: F_CPU / 64, 31250 at 2MHz.
  static constexpr uint16_t COUNTS_PER_SECOND = F_CPU / 64;

  static void begin();

  // Returns the actual number of seconds since the last call
  static uint8_t sleepOneSecond();
};
