#include "WeatherStation.h"

void WeatherStation::addSeconds(uint8_t elapsed_seconds, uint8_t wind_pulses, uint16_t vane_counts,
                                uint8_t rain_tips) {
  _uptime_seconds += elapsed_seconds;
  _sequence++;
  _wind.addSecond(wind_pulses, vane_counts);
  _rain.addSecond(rain_tips);
}

WeatherRegisters WeatherStation::registers() const {
  WeatherRegisters r;
  r.device_id = WEATHER_DEVICE_ID;
  r.map_version = WEATHER_MAP_VERSION;
  r.sequence = _sequence;

  r.status = 0;
  if (_wind.windowFull()) r.status |= WEATHER_STATUS_WIND_VALID;
  if (_wind.vaneFault()) r.status |= WEATHER_STATUS_VANE_FAULT;
  if (_watchdog_reset) r.status |= WEATHER_STATUS_WATCHDOG_RESET;
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
