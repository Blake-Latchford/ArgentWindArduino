#include <unity.h>

#include "src/RainGauge.h"
#include "src/VaneDecoder.h"
#include "src/WindSensor.h"
#include "WeatherStation.h"

void setUp() { WeatherStation::reset(); }
void tearDown() {}

static const uint16_t EAST = VaneDecoder::expected10bitCounts(Heading::East);
static const uint16_t OPEN = 1023;

static void test_initial_registers() {
  WeatherRegisters r = WeatherStation::currentRegisters();
  TEST_ASSERT_EQUAL_HEX8(WEATHER_DEVICE_ID, r.device_id);
  TEST_ASSERT_EQUAL_UINT8(WEATHER_MAP_VERSION, r.map_version);
  TEST_ASSERT_EQUAL_HEX8(0, r.status);
  TEST_ASSERT_EQUAL_UINT8(0, r.sequence);
  TEST_ASSERT_EQUAL_UINT32(0, r.uptime_seconds);
}

static void test_registers_carry_sensor_values() {
  for (int i = 0; i < WIND_WINDOW_SECONDS; i++) WeatherStation::add({3, uint8_t(i < 2 ? 1 : 0), EAST});
  WeatherRegisters r = WeatherStation::currentRegisters();
  TEST_ASSERT_EQUAL_UINT8((uint8_t)WIND_WINDOW_SECONDS, r.sequence);
  TEST_ASSERT_EQUAL_UINT32(WIND_WINDOW_SECONDS, r.uptime_seconds);
  TEST_ASSERT_EQUAL_UINT16(200, r.wind_avg);
  TEST_ASSERT_EQUAL_UINT16(200, r.wind_gust);
  TEST_ASSERT_EQUAL_UINT16(0x4000, r.wind_direction);
  TEST_ASSERT_EQUAL_UINT16(0, r.rain_last_hour);   // rain needs a full hour
  TEST_ASSERT_EQUAL_HEX8(WEATHER_STATUS_WIND_VALID | WEATHER_STATUS_DIRECTION_VALID, r.status);
}

static void test_rain_after_an_hour() {
  for (int i = 0; i < RAIN_HOUR_MINUTES * 60; i++) WeatherStation::add({3, uint8_t(i < 2 ? 1 : 0), EAST});
  WeatherRegisters r = WeatherStation::currentRegisters();
  TEST_ASSERT_EQUAL_UINT16(56, r.rain_last_hour);
  TEST_ASSERT_EQUAL_HEX8(WEATHER_STATUS_WIND_VALID | WEATHER_STATUS_DIRECTION_VALID | WEATHER_STATUS_RAIN_VALID,
                         r.status);
}

static void test_variable_direction_bit() {
  const uint16_t vane[] = {VaneDecoder::expected10bitCounts(Heading::North), EAST,
                           VaneDecoder::expected10bitCounts(Heading::South)};
  for (int i = 0; i < WIND_WINDOW_SECONDS; i++) WeatherStation::add({3, 0, vane[i % 3]});
  TEST_ASSERT_EQUAL_HEX8(
      WEATHER_STATUS_WIND_VALID | WEATHER_STATUS_DIRECTION_VALID | WEATHER_STATUS_DIRECTION_VARIABLE,
      WeatherStation::currentRegisters().status);
}

static void test_status_bits() {
  for (int i = 0; i < WIND_WINDOW_SECONDS; i++) WeatherStation::add({0, 0, OPEN});
  TEST_ASSERT_EQUAL_HEX8(WEATHER_STATUS_WIND_VALID, WeatherStation::currentRegisters().status);
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_initial_registers);
  RUN_TEST(test_registers_carry_sensor_values);
  RUN_TEST(test_rain_after_an_hour);
  RUN_TEST(test_variable_direction_bit);
  RUN_TEST(test_status_bits);
  return UNITY_END();
}
