#pragma once

#include <stdint.h>

// The reported values, as one packed 18-byte block: the I2C register map
// All multi-byte values are little-endian

#define WEATHER_DEVICE_ID 0x57        // 'W'
#define WEATHER_MAP_VERSION 1

// STATUS bits
#define WEATHER_STATUS_WIND_VALID 0x01     // a full wind window is in; until then wind values read 0
#define WEATHER_STATUS_DIRECTION_VALID 0x02
#define WEATHER_STATUS_RAIN_VALID 0x04     // a full rain hour is in; until then rain reads 0

struct __attribute__((packed)) WeatherRegisters {
  uint8_t device_id;            // 0x00  WEATHER_DEVICE_ID
  uint8_t map_version;          // 0x01  WEATHER_MAP_VERSION
  uint8_t status;               // 0x02  WEATHER_STATUS_* bits
  uint8_t sequence;             // 0x03  increments on every 1 s update
  uint16_t wind_avg;            // 0x04  mean speed over the window, 0.01 m/s
  uint16_t wind_gust;           // 0x06  highest rolling 3 s mean in the window, 0.01 m/s
  uint16_t wind_direction;      // 0x08  speed-weighted mean heading, 1/65536 turn (0x4000 is east)
  // 0x0A  Yamartino standard deviation (EPA-454/R-99-005) of the headings, 1/65536 turn
  // 0 when steady; tops out at about 104 degrees (0x49E7) when the headings cancel.
  uint16_t wind_direction_stddev;
  uint16_t rain_last_hour;      // 0x0C  rainfall over the last 60 complete minutes, 0.01 mm
  uint32_t uptime_seconds;      // 0x0E
};

static_assert(sizeof(WeatherRegisters) == 0x12, "register map layout changed");
