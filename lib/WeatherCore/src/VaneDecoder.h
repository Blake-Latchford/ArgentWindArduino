#pragma once

#include <stdint.h>

// Decodes the Argent Data wind vane: eight reed switches with resistors, 16 positions (a magnet can close
// two at once), read as the lower leg of a divider under a 10k resistor.
//
// Assumes the readings are ratiometric, and that the ADC reference voltage is the same as the divider's supply.

// The 16 compass points, clockwise from north in 22.5 degree steps. The value is the step count, so
// (uint8_t)heading * 22.5 is degrees.
enum class Heading : uint8_t {
  North,
  NorthNortheast,
  Northeast,
  EastNortheast,
  East,
  EastSoutheast,
  Southeast,
  SouthSoutheast,
  South,
  SouthSouthwest,
  Southwest,
  WestSouthwest,
  West,
  WestNorthwest,
  Northwest,
  NorthNorthwest,
  Invalid = 0xFF,   // open circuit (magnet between two reeds, or vane unplugged) or short
};

#define HEADING_COUNT 16

class VaneDecoder {
public:
  // The heading whose expected counts are nearest `counts`
  // Returns Heading::Invalid if the counts do not correspond to a valid heading.
  static Heading decode10bitAdc(uint16_t counts);

  // Expected 10-bit counts for `heading`, for inspecting the table.
  static uint16_t expected10bitCounts(Heading heading);
};
