#include <unity.h>

#include "PulseCounter.h"

void setUp() {}
void tearDown() {}

static void count(PulseCounter& c, int n) {
  for (int i = 0; i < n; i++) c.increment();
}

static void test_take_returns_counts_since_last_take() {
  PulseCounter c;
  TEST_ASSERT_EQUAL_UINT8(0, c.take());
  count(c, 5);
  TEST_ASSERT_EQUAL_UINT8(5, c.take());
  TEST_ASSERT_EQUAL_UINT8(0, c.take());
  count(c, 2);
  TEST_ASSERT_EQUAL_UINT8(2, c.take());
}

static void test_total_wrapping_between_takes() {
  PulseCounter c;
  count(c, 250);
  TEST_ASSERT_EQUAL_UINT8(250, c.take());
  count(c, 10);   // the total wraps past 255
  TEST_ASSERT_EQUAL_UINT8(10, c.take());
}

static void test_more_than_255_between_takes_wraps() {
  PulseCounter c;
  count(c, 256 + 3);
  TEST_ASSERT_EQUAL_UINT8(3, c.take());
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_take_returns_counts_since_last_take);
  RUN_TEST(test_total_wrapping_between_takes);
  RUN_TEST(test_more_than_255_between_takes_wraps);
  return UNITY_END();
}
