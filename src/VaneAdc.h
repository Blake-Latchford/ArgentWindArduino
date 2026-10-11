#pragma once

#include <stdint.h>

class VaneAdc {
 public:
  VaneAdc() = delete;

  static void begin();

  // Power the voltage divider
  // and take one 10-bit ratiometric reading
  static uint16_t sample();
};
