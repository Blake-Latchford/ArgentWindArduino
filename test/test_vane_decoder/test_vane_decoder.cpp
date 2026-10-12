#include <unity.h>

#include "src/VaneDecoder.h"

void setUp() {}
void tearDown() {}

#define TEST_ASSERT_EQUAL_HEADING(expected, actual) TEST_ASSERT_EQUAL_UINT8((uint8_t)(expected), (uint8_t)(actual))

static void test_exact_counts_decode_to_their_heading() {
  for (uint8_t i = 0; i < HEADING_COUNT; i++) {
    TEST_ASSERT_EQUAL_HEADING(i, VaneDecoder::decode10bitAdc(VaneDecoder::expected10bitCounts((Heading)i)));
  }
}

static void test_nearby_counts_decode_to_nearest_heading() {
  TEST_ASSERT_EQUAL_HEADING(Heading::East, VaneDecoder::decode10bitAdc(95));   // East is 93, EastNortheast is 84
  TEST_ASSERT_EQUAL_HEADING(Heading::EastNortheast, VaneDecoder::decode10bitAdc(86));
  TEST_ASSERT_EQUAL_HEADING(Heading::West, VaneDecoder::decode10bitAdc(950));
}

static void test_open_circuit_is_invalid() {
  TEST_ASSERT_EQUAL_HEADING(Heading::Invalid, VaneDecoder::decode10bitAdc(1023));
  TEST_ASSERT_EQUAL_HEADING(Heading::Invalid, VaneDecoder::decode10bitAdc(990));
}

static void test_short_is_invalid() {
  TEST_ASSERT_EQUAL_HEADING(Heading::Invalid, VaneDecoder::decode10bitAdc(0));
  TEST_ASSERT_EQUAL_HEADING(Heading::Invalid, VaneDecoder::decode10bitAdc(20));
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_exact_counts_decode_to_their_heading);
  RUN_TEST(test_nearby_counts_decode_to_nearest_heading);
  RUN_TEST(test_open_circuit_is_invalid);
  RUN_TEST(test_short_is_invalid);
  return UNITY_END();
}
