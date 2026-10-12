#include "WeatherStation.h"

#include "RainGauge.h"
#include "VaneDecoder.h"
#include "WindSensor.h"

static WindSensor wind;
static RainGauge rain;
static uint32_t uptime_seconds = 0;
static uint8_t sequence = 0;

void WeatherStation::add(const WeatherSample& sample) {
  uptime_seconds++;
  sequence++;
  wind.addSample({sample.wind_pulses, VaneDecoder::decode10bitAdc(sample.vane_10bit_adc)});
  rain.addSample(sample.rain_tips);
}

WeatherRegisters WeatherStation::currentRegisters() {
  WeatherRegisters r;
  r.device_id = WEATHER_DEVICE_ID;
  r.map_version = WEATHER_MAP_VERSION;
  r.sequence = sequence;

  r.status = 0;
  if (wind.windowFull()) r.status |= WEATHER_STATUS_WIND_VALID;
  if (rain.hourFull()) r.status |= WEATHER_STATUS_RAIN_VALID;

  r.wind_avg = wind.averageSpeed();
  r.wind_gust = wind.gustSpeed();
  
  const WindSensor::Direction direction = wind.direction();
  if (direction.valid) r.status |= WEATHER_STATUS_DIRECTION_VALID;
  if (direction.variable) r.status |= WEATHER_STATUS_DIRECTION_VARIABLE;
  r.wind_direction = direction.angle;

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