#include <unity.h>

#include "src/VaneDecoder.h"
#include "src/WindSensor.h"

void setUp() {}
void tearDown() {}

#define NORTH Heading::North
#define NNE Heading::NorthNortheast
#define EAST Heading::East
#define SOUTH Heading::South
#define WEST Heading::West
#define NNW Heading::NorthNorthwest
#define OPEN Heading::Invalid

static void test_empty_window() {
  WindSensor w;
  TEST_ASSERT_FALSE(w.windowFull());
  TEST_ASSERT_EQUAL_UINT16(0, w.averageSpeed());
  TEST_ASSERT_EQUAL_UINT16(0, w.gustSpeed());
  TEST_ASSERT_FALSE(w.direction().valid);
  TEST_ASSERT_FALSE(w.direction().variable);
  TEST_ASSERT_EQUAL_UINT16(0, w.direction().angle);
}

static void test_nothing_until_the_window_is_full() {
  WindSensor w;
  for (int i = 0; i < WIND_WINDOW_SECONDS - 1; i++) w.addSample({9, EAST});
  TEST_ASSERT_FALSE(w.windowFull());
  TEST_ASSERT_EQUAL_UINT16(0, w.averageSpeed());
  TEST_ASSERT_EQUAL_UINT16(0, w.gustSpeed());
  TEST_ASSERT_FALSE(w.direction().valid);

  w.addSample({9, EAST});
  TEST_ASSERT_TRUE(w.windowFull());
  TEST_ASSERT_EQUAL_UINT16(600, w.averageSpeed());
  TEST_ASSERT_EQUAL_UINT16(600, w.gustSpeed());
  TEST_ASSERT_TRUE(w.direction().valid);
  TEST_ASSERT_EQUAL_UINT16(0x4000, w.direction().angle);
}

static void test_one_pulse_per_second_is_two_thirds_meter_per_second() {
  WindSensor w;
  for (int i = 0; i < WIND_WINDOW_SECONDS; i++) w.addSample({1, NORTH});
  TEST_ASSERT_EQUAL_UINT16(67, w.averageSpeed());
  TEST_ASSERT_EQUAL_UINT16(67, w.gustSpeed());
}

static void test_average_covers_only_the_window() {
  WindSensor w;
  for (int i = 0; i < WIND_WINDOW_SECONDS; i++) w.addSample({9, NORTH});
  TEST_ASSERT_EQUAL_UINT16(600, w.averageSpeed());
  for (int i = 0; i < WIND_WINDOW_SECONDS; i++) w.addSample({3, NORTH});
  TEST_ASSERT_EQUAL_UINT16(200, w.averageSpeed());
}

// A 3 s burst straddling any alignment should read at full speed, since the gust window rolls.
static void test_gust_is_highest_rolling_three_second_mean() {
  WindSensor w;
  for (int i = 0; i < WIND_WINDOW_SECONDS; i++) w.addSample({uint8_t(i == 50 || i == 51 || i == 52 ? 30 : 3), NORTH});
  TEST_ASSERT_EQUAL_UINT16(2000, w.gustSpeed());

  WindSensor w2;
  for (int i = 0; i < WIND_WINDOW_SECONDS; i++) w2.addSample({uint8_t(i == 51 || i == 52 || i == 53 ? 30 : 3), NORTH});
  TEST_ASSERT_EQUAL_UINT16(2000, w2.gustSpeed());
}

static void test_gust_expires_with_the_window() {
  WindSensor w;
  w.addSample({30, NORTH});
  w.addSample({30, NORTH});
  w.addSample({30, NORTH});
  for (int i = 0; i < WIND_WINDOW_SECONDS; i++) w.addSample({3, NORTH});
  TEST_ASSERT_EQUAL_UINT16(200, w.gustSpeed());
}

static void test_gust_across_ring_wraparound() {
  WindSensor w;
  for (int i = 0; i < WIND_WINDOW_SECONDS - 1; i++) w.addSample({0, NORTH});
  w.addSample({15, NORTH});   // last slot of the ring
  w.addSample({15, NORTH});   // wraps to the first
  w.addSample({15, NORTH});
  TEST_ASSERT_EQUAL_UINT16(1000, w.gustSpeed());
}

static void test_direction_is_speed_weighted() {
  WindSensor w;
  // Strong wind from the east, a brief light spell from the west.
  for (int i = 0; i < WIND_WINDOW_SECONDS / 2; i++) w.addSample({10, EAST});
  for (int i = 0; i < WIND_WINDOW_SECONDS / 2; i++) w.addSample({1, WEST});
  TEST_ASSERT_TRUE(w.direction().valid);
  TEST_ASSERT_EQUAL_UINT16(0x4000, w.direction().angle);
}

static void test_direction_averages_across_north() {
  WindSensor w;
  for (int i = 0; i < WIND_WINDOW_SECONDS / 2; i++) w.addSample({5, NNW});
  for (int i = 0; i < WIND_WINDOW_SECONDS / 2; i++) w.addSample({5, NNE});
  TEST_ASSERT_TRUE(w.direction().valid);
  TEST_ASSERT_EQUAL_UINT16(0, w.direction().angle);
}

// Just west of north is near the top of the range, not negative.
static void test_direction_west_of_north_wraps_high() {
  WindSensor w;
  for (int i = 0; i < WIND_WINDOW_SECONDS; i++) w.addSample({5, NNW});
  TEST_ASSERT_TRUE(w.direction().valid);
  // 337.5 degrees. The x1000 sin/cos table puts off-axis headings a few counts (hundredths of a degree) off.
  TEST_ASSERT_UINT16_WITHIN(8, 0xF000, w.direction().angle);
}

static void test_steady_direction_is_not_variable() {
  WindSensor w;
  for (int i = 0; i < WIND_WINDOW_SECONDS; i++) w.addSample({5, EAST});
  TEST_ASSERT_FALSE(w.direction().variable);
}

// North, east and south equally: the resultant is a third of the pulses' length, but still points east.
static void test_wide_spread_is_variable() {
  WindSensor w;
  const Heading headings[] = {NORTH, EAST, SOUTH};
  for (int i = 0; i < WIND_WINDOW_SECONDS; i++) w.addSample({5, headings[i % 3]});
  TEST_ASSERT_TRUE(w.direction().valid);
  TEST_ASSERT_EQUAL_UINT16(0x4000, w.direction().angle);
  TEST_ASSERT_TRUE(w.direction().variable);
}

// The threshold is an even spread over half the compass. The nine points from west through north to east
// (180 degrees) are variable; the seven from WNW to ENE (135 degrees) are not.
static void test_variable_threshold_is_half_the_compass() {
  WindSensor half;
  for (int i = 0; i < WIND_WINDOW_SECONDS; i++) {
    half.addSample({5, Heading((12 + i % 9) % HEADING_COUNT)});   // W through N to E
  }
  TEST_ASSERT_TRUE(half.direction().variable);

  WindSensor narrower;
  for (int i = 0; i < WIND_WINDOW_SECONDS; i++) {
    narrower.addSample({5, Heading((13 + i % 7) % HEADING_COUNT)});   // WNW through N to ENE
  }
  TEST_ASSERT_FALSE(narrower.direction().variable);
}

// Seconds without a heading don't count towards the threshold, so a mostly faulty vane can still be steady.
static void test_variable_ignores_invalid_headings() {
  WindSensor w;
  for (int i = 0; i < WIND_WINDOW_SECONDS; i++) w.addSample({5, i % 10 == 0 ? EAST : OPEN});
  TEST_ASSERT_FALSE(w.direction().variable);
}

static void test_no_direction_in_calm() {
  WindSensor w;
  for (int i = 0; i < WIND_WINDOW_SECONDS; i++) w.addSample({0, SOUTH});
  TEST_ASSERT_FALSE(w.direction().valid);
}

static void test_invalid_vane_samples_are_ignored() {
  WindSensor w;
  for (int i = 0; i < WIND_WINDOW_SECONDS; i++) w.addSample({5, OPEN});
  TEST_ASSERT_FALSE(w.direction().valid);
  TEST_ASSERT_EQUAL_UINT16(333, w.averageSpeed());   // speed still counts

  w.addSample({5, WEST});
  TEST_ASSERT_TRUE(w.direction().valid);
  TEST_ASSERT_EQUAL_UINT16(0xC000, w.direction().angle);
}

static void test_direction_forgets_evicted_samples() {
  WindSensor w;
  for (int i = 0; i < WIND_WINDOW_SECONDS; i++) w.addSample({20, EAST});
  for (int i = 0; i < WIND_WINDOW_SECONDS; i++) w.addSample({1, WEST});
  TEST_ASSERT_TRUE(w.direction().valid);
  TEST_ASSERT_EQUAL_UINT16(0xC000, w.direction().angle);
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
  RUN_TEST(test_direction_west_of_north_wraps_high);
  RUN_TEST(test_steady_direction_is_not_variable);
  RUN_TEST(test_wide_spread_is_variable);
  RUN_TEST(test_variable_threshold_is_half_the_compass);
  RUN_TEST(test_variable_ignores_invalid_headings);
  RUN_TEST(test_no_direction_in_calm);
  RUN_TEST(test_invalid_vane_samples_are_ignored);
  RUN_TEST(test_direction_forgets_evicted_samples);
  return UNITY_END();
}
