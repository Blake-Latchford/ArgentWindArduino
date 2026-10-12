#include "VaneDecoder.h"

namespace {

// Expected 10-bit ADC counts per Heading, computed as 1024 * R / (R + 10k) from the nominal datasheet
// resistances. These are placeholders: replace them with counts measured on the installed vane and cable
// (the debug build prints vane_raw each second; see README.md).
const uint16_t VANE_COUNTS[HEADING_COUNT] = {
  786,  //   0.0  33k
  406,  //  22.5  6.57k
  461,  //  45.0  8.2k
   84,  //  67.5  891
   93,  //  90.0  1k
   66,  // 112.5  688
  185,  // 135.0  2.2k
  127,  // 157.5  1.41k
  287,  // 180.0  3.9k
  245,  // 202.5  3.14k
  630,  // 225.0  16k
  599,  // 247.5  14.12k
  945,  // 270.0  120k
  828,  // 292.5  42.12k
  887,  // 315.0  64.9k
  703,  // 337.5  21.88k
};

// Readings outside this range can't be any position: above is an open circuit (reads ~1023), below is a
// short. Nominally twice the highest and half the lowest resistance in the table.
const uint16_t VANE_OPEN_COUNTS = 983;   // 240k
const uint16_t VANE_SHORT_COUNTS = 34;   // 344 ohms

}  // namespace

uint16_t VaneDecoder::expected10bitCounts(Heading heading) { return VANE_COUNTS[(uint8_t)heading]; }

Heading VaneDecoder::decode10bitAdc(uint16_t counts) {
  if (counts > VANE_OPEN_COUNTS || counts < VANE_SHORT_COUNTS) {
    return Heading::Invalid;
  }

  uint8_t best = 0;
  uint16_t best_distance = 0xFFFF;
  for (uint8_t i = 0; i < HEADING_COUNT; i++) {
    uint16_t distance = counts > VANE_COUNTS[i] ? counts - VANE_COUNTS[i] : VANE_COUNTS[i] - counts;
    if (distance < best_distance) {
      best_distance = distance;
      best = i;
    }
  }
  return (Heading)best;
}
