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
  TEST_ASSERT_EQUAL_UINT16(0, w.direction().angle);
  TEST_ASSERT_EQUAL_UINT16(0, w.direction().stddev);
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

// Wherever the ring's seam falls, a burst at either end of the window counts, and the newest second
// isn't summed with the oldest two.
static void test_gust_at_every_seam_position() {
  const uint16_t extras[] = {0, 1, 2, 3, 4, 300, WIND_WINDOW_SECONDS - 3, WIND_WINDOW_SECONDS - 2,
                             WIND_WINDOW_SECONDS - 1};
  for (uint16_t extra : extras) {
    // Newest three seconds.
    WindSensor newest;
    for (uint16_t i = 0; i < WIND_WINDOW_SECONDS + extra; i++) {
      newest.addSample({uint8_t(i >= WIND_WINDOW_SECONDS + extra - 3 ? 30 : 3), NORTH});
    }
    TEST_ASSERT_EQUAL_UINT16(2000, newest.gustSpeed());

    // Oldest three seconds.
    WindSensor oldest;
    for (uint16_t i = 0; i < WIND_WINDOW_SECONDS + extra; i++) {
      oldest.addSample({uint8_t(i >= extra && i < extra + 3 ? 30 : 3), NORTH});
    }
    TEST_ASSERT_EQUAL_UINT16(2000, oldest.gustSpeed());

    // The newest second and the oldest two are strong. Summed across the seam they'd read 2000, but the
    // real gust is the oldest two and the one after: 30 + 30 + 3 over 3 s.
    WindSensor seam;
    for (uint16_t i = 0; i < WIND_WINDOW_SECONDS + extra; i++) {
      bool strong = i == WIND_WINDOW_SECONDS + extra - 1 || i == extra || i == extra + 1;
      seam.addSample({uint8_t(strong ? 30 : 3), NORTH});
    }
    TEST_ASSERT_EQUAL_UINT16(1400, seam.gustSpeed());
  }
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
  TEST_ASSERT_EQUAL_UINT16(0xF000, w.direction().angle);   // 337.5 degrees
}

static void test_steady_direction_has_no_spread() {
  WindSensor w;
  for (int i = 0; i < WIND_WINDOW_SECONDS; i++) w.addSample({5, EAST});
  TEST_ASSERT_EQUAL_UINT16(0, w.direction().stddev);

  // Off the axes too, where table rounding would leave a floor with a coarser scale.
  WindSensor ne;
  for (int i = 0; i < WIND_WINDOW_SECONDS; i++) ne.addSample({5, Heading::Northeast});
  TEST_ASSERT_EQUAL_UINT16(0, ne.direction().stddev);
}

// NNW and NNE equally: 22.5 degrees either side of north. Yamartino reads a touch high, about 22.7 degrees.
static void test_stddev_of_two_neighbours() {
  WindSensor w;
  for (int i = 0; i < WIND_WINDOW_SECONDS; i++) w.addSample({5, i % 2 ? NNE : NNW});
  TEST_ASSERT_EQUAL_UINT16(4131, w.direction().stddev);
}

// North, east and south equally: the mean still points east, about 79.7 degrees either way.
static void test_stddev_of_a_wide_spread() {
  WindSensor w;
  const Heading headings[] = {NORTH, EAST, SOUTH};
  for (int i = 0; i < WIND_WINDOW_SECONDS; i++) w.addSample({5, headings[i % 3]});
  TEST_ASSERT_TRUE(w.direction().valid);
  TEST_ASSERT_EQUAL_UINT16(0x4000, w.direction().angle);
  TEST_ASSERT_EQUAL_UINT16(14504, w.direction().stddev);
}

// The mean is speed-weighted but the spread isn't. Strong east and light west for equal times point east,
// and the unit vectors cancel, which is Yamartino's maximum of about 104 degrees.
static void test_stddev_is_not_speed_weighted() {
  WindSensor w;
  for (int i = 0; i < WIND_WINDOW_SECONDS; i++) w.addSample({uint8_t(i % 2 ? 10 : 1), i % 2 ? EAST : WEST});
  TEST_ASSERT_EQUAL_UINT16(0x4000, w.direction().angle);
  TEST_ASSERT_EQUAL_UINT16(18919, w.direction().stddev);
}

// Seconds without a heading don't count, so a mostly faulty vane can still be steady.
static void test_stddev_ignores_invalid_headings() {
  WindSensor w;
  for (int i = 0; i < WIND_WINDOW_SECONDS; i++) w.addSample({5, i % 10 == 0 ? EAST : OPEN});
  TEST_ASSERT_EQUAL_UINT16(0, w.direction().stddev);
}

// The spread forgets old seconds along with the mean.
static void test_stddev_forgets_evicted_samples() {
  WindSensor w;
  for (int i = 0; i < WIND_WINDOW_SECONDS; i++) w.addSample({5, i % 2 ? EAST : WEST});
  for (int i = 0; i < WIND_WINDOW_SECONDS; i++) w.addSample({5, NORTH});
  TEST_ASSERT_EQUAL_UINT16(0, w.direction().stddev);
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
  TEST_ASSERT_EQUAL_UINT16(0, w.direction().angle);
  TEST_ASSERT_EQUAL_UINT16(0, w.direction().stddev);
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
  RUN_TEST(test_gust_at_every_seam_position);
  RUN_TEST(test_direction_is_speed_weighted);
  RUN_TEST(test_direction_averages_across_north);
  RUN_TEST(test_direction_west_of_north_wraps_high);
  RUN_TEST(test_steady_direction_has_no_spread);
  RUN_TEST(test_stddev_of_two_neighbours);
  RUN_TEST(test_stddev_of_a_wide_spread);
  RUN_TEST(test_stddev_is_not_speed_weighted);
  RUN_TEST(test_stddev_ignores_invalid_headings);
  RUN_TEST(test_stddev_forgets_evicted_samples);
  RUN_TEST(test_no_direction_in_calm);
  RUN_TEST(test_invalid_vane_samples_are_ignored);
  RUN_TEST(test_direction_forgets_evicted_samples);
  return UNITY_END();
}
