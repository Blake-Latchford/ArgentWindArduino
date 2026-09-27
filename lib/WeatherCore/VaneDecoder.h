#pragma once

#include <stdint.h>

// Decodes the Argent Data wind vane: eight reed switches with resistors, 16 positions (a magnet can close
// two at once), read as the lower leg of a divider under a 10k resistor driven from VCC. The ADC uses VCC
// as its reference, so the reading is ratiometric and the table is in raw 10-bit counts.

#define VANE_POSITION_COUNT 16
#define VANE_INVALID 0xFF

class VaneDecoder {
public:
  // Heading index (0 = north, each step 22.5 degrees clockwise) nearest to `counts`, or VANE_INVALID for
  // an open circuit (magnet between two reeds, or vane unplugged) or a short.
  static uint8_t decode(uint16_t counts);

  // Expected counts for heading index `i`, for inspecting the table.
  static uint16_t expectedCounts(uint8_t i);
};
