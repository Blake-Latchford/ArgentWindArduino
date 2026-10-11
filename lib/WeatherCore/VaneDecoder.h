#pragma once

#include <stdint.h>

// Decodes the Argent Data wind vane: eight reed switches with resistors, 16 positions (a magnet can close
// two at once), read as the lower leg of a divider under a 10k resistor driven from VCC.
//
// Readings are raw ADC counts: the 10-bit conversion result, 0-1023, where 1023 is VCC. The ADC uses VCC as
// its reference, so a resistance R always reads 1024 * R / (R + 10k) whatever the supply voltage is, and
// the table can be in counts without converting to volts.

// The 16 compass points, clockwise from north in 22.5 degree steps. The value is the step count, so
// (uint8_t)heading * 22.5 is degrees. The names are spelled out because the AVR headers define SE.
enum class Heading : uint8_t {
  North, NorthNortheast, Northeast, EastNortheast,
  East, EastSoutheast, Southeast, SouthSoutheast,
  South, SouthSouthwest, Southwest, WestSouthwest,
  West, WestNorthwest, Northwest, NorthNorthwest,
  Invalid = 0xFF,   // open circuit (magnet between two reeds, or vane unplugged) or short
};

#define HEADING_COUNT 16

class VaneDecoder {
public:
  // The heading whose expected counts are nearest `counts`, or Heading::Invalid for an open circuit or a
  // short.
  static Heading decode(uint16_t counts);

  // Expected ADC counts for `heading`, for inspecting the table.
  static uint16_t expectedCounts(Heading heading);
};
