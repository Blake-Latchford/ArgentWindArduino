#include <unity.h>

#include "VaneDecoder.h"

void setUp() {}
void tearDown() {}

static void test_exact_counts_decode_to_their_heading() {
  for (uint8_t i = 0; i < VANE_POSITION_COUNT; i++) {
    TEST_ASSERT_EQUAL_UINT8(i, VaneDecoder::decode(VaneDecoder::expectedCounts(i)));
  }
}

static void test_nearby_counts_decode_to_nearest_heading() {
  TEST_ASSERT_EQUAL_UINT8(4, VaneDecoder::decode(95));   // 90 deg is 93, 67.5 deg is 84
  TEST_ASSERT_EQUAL_UINT8(3, VaneDecoder::decode(86));
  TEST_ASSERT_EQUAL_UINT8(12, VaneDecoder::decode(950));
}

static void test_open_circuit_is_invalid() {
  TEST_ASSERT_EQUAL_UINT8(VANE_INVALID, VaneDecoder::decode(1023));
  TEST_ASSERT_EQUAL_UINT8(VANE_INVALID, VaneDecoder::decode(990));
}

static void test_short_is_invalid() {
  TEST_ASSERT_EQUAL_UINT8(VANE_INVALID, VaneDecoder::decode(0));
  TEST_ASSERT_EQUAL_UINT8(VANE_INVALID, VaneDecoder::decode(20));
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_exact_counts_decode_to_their_heading);
  RUN_TEST(test_nearby_counts_decode_to_nearest_heading);
  RUN_TEST(test_open_circuit_is_invalid);
  RUN_TEST(test_short_is_invalid);
  return UNITY_END();
}
