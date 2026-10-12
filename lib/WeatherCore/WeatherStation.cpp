#include "WeatherStation.h"

void WeatherStation::add(const WeatherSample& sample) {
  _uptime_seconds += sample.seconds;
  _sequence++;
  _wind.addSecond({sample.wind_pulses, VaneDecoder::decode10bitAdc(sample.vane_10bit_adc)});
  _rain.addSecond(sample.rain_tips);
}

WeatherRegisters WeatherStation::currentRegisters() const {
  WeatherRegisters r;
  r.device_id = WEATHER_DEVICE_ID;
  r.map_version = WEATHER_MAP_VERSION;
  r.sequence = _sequence;

  r.status = 0;
  if (_wind.windowFull()) r.status |= WEATHER_STATUS_WIND_VALID;
  if (_wind.vaneFault()) r.status |= WEATHER_STATUS_VANE_FAULT;
  if (_rain.hourFull()) r.status |= WEATHER_STATUS_RAIN_VALID;

  r.wind_avg = _wind.averageSpeed();
  r.wind_gust = _wind.gustSpeed();
  
  uint16_t unaligned_direction_buffer = 0;
  if (_wind.direction(unaligned_direction_buffer)) r.status |= WEATHER_STATUS_DIRECTION_VALID;
  r.wind_direction = unaligned_direction_buffer;

  r.rain_last_hour = _rain.lastHourRain();
  r.uptime_seconds = _uptime_seconds;
  return r;
}
