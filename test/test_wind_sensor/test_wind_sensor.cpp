#include <unity.h>

#include "VaneDecoder.h"
#include "WindSensor.h"

void setUp() {}
void tearDown() {}

// Vane ADC readings for each heading, and for an open circuit.
#define NORTH VaneDecoder::expectedCounts(0)
#define NNE VaneDecoder::expectedCounts(1)
#define EAST VaneDecoder::expectedCounts(4)
#define SOUTH VaneDecoder::expectedCounts(8)
#define WEST VaneDecoder::expectedCounts(12)
#define NNW VaneDecoder::expectedCounts(15)
#define OPEN 1023

static void test_empty_window() {
  WindSensor w;
  uint16_t dir = 1234;
  TEST_ASSERT_FALSE(w.windowFull());
  TEST_ASSERT_EQUAL_UINT16(0, w.averageSpeed());
  TEST_ASSERT_EQUAL_UINT16(0, w.gustSpeed());
  TEST_ASSERT_FALSE(w.direction(dir));
  TEST_ASSERT_EQUAL_UINT16(1234, dir);
}

static void test_nothing_until_the_window_is_full() {
  WindSensor w;
  for (int i = 0; i < WIND_WINDOW_SECONDS - 1; i++) w.addSecond(9, EAST);
  uint16_t dir = 1234;
  TEST_ASSERT_FALSE(w.windowFull());
  TEST_ASSERT_EQUAL_UINT16(0, w.averageSpeed());
  TEST_ASSERT_EQUAL_UINT16(0, w.gustSpeed());
  TEST_ASSERT_FALSE(w.direction(dir));
  TEST_ASSERT_EQUAL_UINT16(1234, dir);

  w.addSecond(9, EAST);
  TEST_ASSERT_TRUE(w.windowFull());
  TEST_ASSERT_EQUAL_UINT16(600, w.averageSpeed());
  TEST_ASSERT_EQUAL_UINT16(600, w.gustSpeed());
  TEST_ASSERT_TRUE(w.direction(dir));
  TEST_ASSERT_EQUAL_UINT16(900, dir);
}

static void test_one_pulse_per_second_is_two_thirds_meter_per_second() {
  WindSensor w;
  for (int i = 0; i < WIND_WINDOW_SECONDS; i++) w.addSecond(1, NORTH);
  TEST_ASSERT_EQUAL_UINT16(67, w.averageSpeed());
  TEST_ASSERT_EQUAL_UINT16(67, w.gustSpeed());
}

static void test_average_covers_only_the_window() {
  WindSensor w;
  for (int i = 0; i < WIND_WINDOW_SECONDS; i++) w.addSecond(9, NORTH);
  TEST_ASSERT_EQUAL_UINT16(600, w.averageSpeed());
  for (int i = 0; i < WIND_WINDOW_SECONDS; i++) w.addSecond(3, NORTH);
  TEST_ASSERT_EQUAL_UINT16(200, w.averageSpeed());
}

// A 3 s burst straddling any alignment should read at full speed, since the gust window rolls.
static void test_gust_is_highest_rolling_three_second_mean() {
  WindSensor w;
  for (int i = 0; i < WIND_WINDOW_SECONDS; i++) w.addSecond(i == 50 || i == 51 || i == 52 ? 30 : 3, NORTH);
  TEST_ASSERT_EQUAL_UINT16(2000, w.gustSpeed());

  WindSensor w2;
  for (int i = 0; i < WIND_WINDOW_SECONDS; i++) w2.addSecond(i == 51 || i == 52 || i == 53 ? 30 : 3, NORTH);
  TEST_ASSERT_EQUAL_UINT16(2000, w2.gustSpeed());
}

static void test_gust_expires_with_the_window() {
  WindSensor w;
  w.addSecond(30, NORTH);
  w.addSecond(30, NORTH);
  w.addSecond(30, NORTH);
  for (int i = 0; i < WIND_WINDOW_SECONDS; i++) w.addSecond(3, NORTH);
  TEST_ASSERT_EQUAL_UINT16(200, w.gustSpeed());
}

static void test_gust_across_ring_wraparound() {
  WindSensor w;
  for (int i = 0; i < WIND_WINDOW_SECONDS - 1; i++) w.addSecond(0, NORTH);
  w.addSecond(15, NORTH);   // last slot of the ring
  w.addSecond(15, NORTH);   // wraps to the first
  w.addSecond(15, NORTH);
  TEST_ASSERT_EQUAL_UINT16(1000, w.gustSpeed());
}

static void test_direction_is_speed_weighted() {
  WindSensor w;
  // Strong wind from the east, a brief light spell from the west.
  for (int i = 0; i < WIND_WINDOW_SECONDS / 2; i++) w.addSecond(10, EAST);
  for (int i = 0; i < WIND_WINDOW_SECONDS / 2; i++) w.addSecond(1, WEST);
  uint16_t dir = 0;
  TEST_ASSERT_TRUE(w.direction(dir));
  TEST_ASSERT_EQUAL_UINT16(900, dir);
}

static void test_direction_averages_across_north() {
  WindSensor w;
  for (int i = 0; i < WIND_WINDOW_SECONDS / 2; i++) w.addSecond(5, NNW);
  for (int i = 0; i < WIND_WINDOW_SECONDS / 2; i++) w.addSecond(5, NNE);
  uint16_t dir = 1234;
  TEST_ASSERT_TRUE(w.direction(dir));
  TEST_ASSERT_EQUAL_UINT16(0, dir);
}

static void test_no_direction_in_calm() {
  WindSensor w;
  for (int i = 0; i < WIND_WINDOW_SECONDS; i++) w.addSecond(0, SOUTH);
  uint16_t dir = 1234;
  TEST_ASSERT_FALSE(w.direction(dir));
  TEST_ASSERT_EQUAL_UINT16(1234, dir);
}

static void test_invalid_vane_samples_are_ignored() {
  WindSensor w;
  for (int i = 0; i < WIND_WINDOW_SECONDS; i++) w.addSecond(5, OPEN);
  uint16_t dir = 1234;
  TEST_ASSERT_TRUE(w.vaneFault());
  TEST_ASSERT_FALSE(w.direction(dir));
  TEST_ASSERT_EQUAL_UINT16(333, w.averageSpeed());   // speed still counts

  w.addSecond(5, WEST);
  TEST_ASSERT_FALSE(w.vaneFault());
  TEST_ASSERT_TRUE(w.direction(dir));
  TEST_ASSERT_EQUAL_UINT16(2700, dir);
}

static void test_direction_forgets_evicted_samples() {
  WindSensor w;
  for (int i = 0; i < WIND_WINDOW_SECONDS; i++) w.addSecond(20, EAST);
  for (int i = 0; i < WIND_WINDOW_SECONDS; i++) w.addSecond(1, WEST);
  uint16_t dir = 0;
  TEST_ASSERT_TRUE(w.direction(dir));
  TEST_ASSERT_EQUAL_UINT16(2700, dir);
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_empty_window);
  RUN_TEST(test_nothing_until_the_window_is_full);
  RUN_TEST(test_one_pulse_per_second_is_two_thirds_meter_per_second);
  RUN_TEST(test_average_covers_only_the_window);
  RUN_TEST(test_gust_is_highest_rolling_three_second_mean);
  RUN_TEST(test_gust_expires_with_the_window);
  RUN_TEST(test_gust_across_ring_wraparound);
  RUN_TEST(test_direction_is_speed_weighted);
  RUN_TEST(test_direction_averages_across_north);
  RUN_TEST(test_no_direction_in_calm);
  RUN_TEST(test_invalid_vane_samples_are_ignored);
  RUN_TEST(test_direction_forgets_evicted_samples);
  return UNITY_END();
}
