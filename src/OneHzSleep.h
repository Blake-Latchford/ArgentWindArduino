#pragma once

#include <stdint.h>

// 1Hz time base from Timer1.
class OneHzSleep {
 public:
  OneHzSleep() = delete;

  static void begin();

  // Idle-sleep until at least one second has elapsed, waking for other interrupts (reed switches, I2C)
  // as they come. Returns the number of seconds since the last call: 1, unless the caller fell behind.
  static uint8_t untilNextSecond();
};
