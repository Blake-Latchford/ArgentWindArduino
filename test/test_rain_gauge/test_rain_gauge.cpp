#include <unity.h>

#include "src/RainGauge.h"

void setUp() {}
void tearDown() {}

#define HOUR_SECONDS (RAIN_HOUR_MINUTES * 60)

static void addSeconds(RainGauge& r, int seconds, uint8_t tips_each) {
  for (int i = 0; i < seconds; i++) r.addSample(tips_each);
}

static void test_starts_dry() {
  RainGauge r;
  TEST_ASSERT_FALSE(r.hourFull());
  TEST_ASSERT_EQUAL_UINT16(0, r.lastHourRain());
}

static void test_nothing_until_the_hour_is_full() {
  RainGauge r;
  r.addSample(1);
  addSeconds(r, HOUR_SECONDS - 2, 0);
  TEST_ASSERT_FALSE(r.hourFull());
  TEST_ASSERT_EQUAL_UINT16(0, r.lastHourRain());

  r.addSample(0);
  TEST_ASSERT_TRUE(r.hourFull());
  TEST_ASSERT_EQUAL_UINT16(28, r.lastHourRain());   // one tip is 0.2794 mm
}

// Converting from the tip count keeps per-tip rounding from adding up.
static void test_hour_does_not_accumulate_rounding() {
  RainGauge r;
  addSeconds(r, 100, 1);
  addSeconds(r, HOUR_SECONDS - 100, 0);
  TEST_ASSERT_EQUAL_UINT16(2794, r.lastHourRain());
}

// The hour is exactly 60 complete minutes: the minute under way isn't in it yet, and a minute leaves the
// hour 60 minutes after it completed.
static void test_hour_is_sixty_complete_minutes() {
  RainGauge r;
  r.addSample(10);                          // minute 0
  addSeconds(r, HOUR_SECONDS - 1, 0);
  TEST_ASSERT_EQUAL_UINT16(279, r.lastHourRain());

  r.addSample(1);                           // minute 60, under way
  TEST_ASSERT_EQUAL_UINT16(279, r.lastHourRain());

  addSeconds(r, 59, 0);                     // minute 60 completes; minute 0 drops out
  TEST_ASSERT_EQUAL_UINT16(28, r.lastHourRain());

  addSeconds(r, HOUR_SECONDS, 0);
  TEST_ASSERT_EQUAL_UINT16(0, r.lastHourRain());
}

static void test_hour_saturates() {
  RainGauge r;
  addSeconds(r, HOUR_SECONDS, 1);           // 3600 tips = 1006 mm
  TEST_ASSERT_EQUAL_UINT16(0xFFFF, r.lastHourRain());
}

static void test_minute_saturates() {
  RainGauge r;
  addSeconds(r, 3, 200);
  addSeconds(r, HOUR_SECONDS - 3, 0);
  TEST_ASSERT_EQUAL_UINT16((255UL * 2794 + 50) / 100, r.lastHourRain());
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_starts_dry);
  RUN_TEST(test_nothing_until_the_hour_is_full);
  RUN_TEST(test_hour_does_not_accumulate_rounding);
  RUN_TEST(test_hour_is_sixty_complete_minutes);
  RUN_TEST(test_hour_saturates);
  RUN_TEST(test_minute_saturates);
  return UNITY_END();
}
