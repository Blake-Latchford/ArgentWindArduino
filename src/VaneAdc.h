#pragma once

#include <stdint.h>

class VaneAdc {
 public:
  VaneAdc() = delete;

  static void begin();

  static uint16_t sample();
};
