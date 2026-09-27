#pragma once

#include <stdint.h>

// Wind vane divider: D4 drives a 10k resistor to A0, and the vane runs from A0 to GND. The divider and
// the ADC are powered only while sampling.
class VaneAdc {
 public:
  VaneAdc() = delete;

  static void begin();

  // The divider's 10-bit ADC reading. The ADC reference is VCC, the same supply as the divider, so the
  // reading is ratiometric.
  static uint16_t sample();
};
