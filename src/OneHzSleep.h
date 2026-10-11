#pragma once

#include <stdint.h>

// 1Hz time base from Timer1.
class OneHzSleep {
 public:
  OneHzSleep() = delete;

  static void begin();

  // Returns the actual number of seconds since the last call
  static uint8_t sleepOneSecond();
};
