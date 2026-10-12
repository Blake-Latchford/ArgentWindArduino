#include "WeatherStation.h"

#include "RainGauge.h"
#include "VaneDecoder.h"
#include "WindSensor.h"

static WindSensor wind;
static RainGauge rain;
static uint32_t uptime_seconds = 0;
static uint8_t sequence = 0;

void WeatherStation::add(const WeatherSample& sample) {
  uptime_seconds += sample.seconds;
  sequence++;
  wind.addSecond({sample.wind_pulses, VaneDecoder::decode10bitAdc(sample.vane_10bit_adc)});
  rain.addSecond(sample.rain_tips);
}

WeatherRegisters WeatherStation::currentRegisters() {
  WeatherRegisters r;
  r.device_id = WEATHER_DEVICE_ID;
  r.map_version = WEATHER_MAP_VERSION;
  r.sequence = sequence;

  r.status = 0;
  if (wind.windowFull()) r.status |= WEATHER_STATUS_WIND_VALID;
  if (wind.vaneFault()) r.status |= WEATHER_STATUS_VANE_FAULT;
  if (rain.hourFull()) r.status |= WEATHER_STATUS_RAIN_VALID;

  r.wind_avg = wind.averageSpeed();
  r.wind_gust = wind.gustSpeed();
  
  uint16_t unaligned_direction_buffer = 0;
  if (wind.direction(unaligned_direction_buffer)) r.status |= WEATHER_STATUS_DIRECTION_VALID;
  r.wind_direction = unaligned_direction_buffer;

  r.rain_last_hour = rain.lastHourRain();
  r.uptime_seconds = uptime_seconds;
  return r;
}

// Reset for unit tests.
void WeatherStation::reset() {
  wind = WindSensor();
  rain = RainGauge();
  uptime_seconds = 0;
  sequence = 0;
}