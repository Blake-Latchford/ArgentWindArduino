#include <unity.h>

#include "VaneDecoder.h"
#include "WeatherStation.h"

void setUp() {}
void tearDown() {}

static const uint16_t EAST = VaneDecoder::expected10bitCounts(Heading::East);
static const uint16_t OPEN = 1023;

static void test_initial_registers() {
  WeatherStation s;
  WeatherRegisters r = s.currentRegisters();
  TEST_ASSERT_EQUAL_HEX8(WEATHER_DEVICE_ID, r.device_id);
  TEST_ASSERT_EQUAL_UINT8(WEATHER_MAP_VERSION, r.map_version);
  TEST_ASSERT_EQUAL_HEX8(0, r.status);
  TEST_ASSERT_EQUAL_UINT8(0, r.sequence);
  TEST_ASSERT_EQUAL_UINT32(0, r.uptime_seconds);
}

static void test_registers_carry_sensor_values() {
  WeatherStation s;
  for (int i = 0; i < WIND_WINDOW_SECONDS; i++) s.add({1, 3, uint8_t(i < 2 ? 1 : 0), EAST});
  WeatherRegisters r = s.currentRegisters();
  TEST_ASSERT_EQUAL_UINT8((uint8_t)WIND_WINDOW_SECONDS, r.sequence);
  TEST_ASSERT_EQUAL_UINT32(WIND_WINDOW_SECONDS, r.uptime_seconds);
  TEST_ASSERT_EQUAL_UINT16(200, r.wind_avg);
  TEST_ASSERT_EQUAL_UINT16(200, r.wind_gust);
  TEST_ASSERT_EQUAL_UINT16(900, r.wind_direction);
  TEST_ASSERT_EQUAL_UINT16(0, r.rain_last_hour);   // rain needs a full hour
  TEST_ASSERT_EQUAL_HEX8(WEATHER_STATUS_WIND_VALID | WEATHER_STATUS_DIRECTION_VALID, r.status);
}

static void test_rain_after_an_hour() {
  WeatherStation s;
  for (int i = 0; i < RAIN_HOUR_MINUTES * 60; i++) s.add({1, 3, uint8_t(i < 2 ? 1 : 0), EAST});
  WeatherRegisters r = s.currentRegisters();
  TEST_ASSERT_EQUAL_UINT16(56, r.rain_last_hour);
  TEST_ASSERT_EQUAL_HEX8(WEATHER_STATUS_WIND_VALID | WEATHER_STATUS_DIRECTION_VALID | WEATHER_STATUS_RAIN_VALID,
                         r.status);
}

static void test_status_bits() {
  WeatherStation s;
  s.setWatchdogReset(true);
  for (int i = 0; i < WIND_WINDOW_SECONDS; i++) s.add({1, 0, 0, OPEN});
  TEST_ASSERT_EQUAL_HEX8(WEATHER_STATUS_WIND_VALID | WEATHER_STATUS_VANE_FAULT | WEATHER_STATUS_WATCHDOG_RESET,
                         s.currentRegisters().status);
}

static void test_missed_ticks_advance_uptime() {
  WeatherStation s;
  s.add({3, 0, 0, OPEN});
  TEST_ASSERT_EQUAL_UINT32(3, s.currentRegisters().uptime_seconds);
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_initial_registers);
  RUN_TEST(test_registers_carry_sensor_values);
  RUN_TEST(test_rain_after_an_hour);
  RUN_TEST(test_status_bits);
  RUN_TEST(test_missed_ticks_advance_uptime);
  return UNITY_END();
}
