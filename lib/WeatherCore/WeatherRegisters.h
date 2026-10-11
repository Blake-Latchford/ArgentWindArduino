#pragma once

#include <stdint.h>

// I2C register map served to the RAK. All multi-byte values are little-endian. The master writes one
// byte to set the register pointer, then reads; a read returns bytes from the pointer to the end of the
// map. The whole map fits in the Wire library's 32-byte buffer, so any read is one consistent snapshot.

#define WEATHER_I2C_PRIMARY_ADDRESS 0x6C
#define WEATHER_I2C_SECONDARY_ADDRESS 0x6E  // selected by the address jumper
#define WEATHER_DEVICE_ID 0x57        // 'W'
#define WEATHER_MAP_VERSION 3

// STATUS bits
#define WEATHER_STATUS_WIND_VALID 0x01     // a full wind window is in; until then wind values read 0
#define WEATHER_STATUS_DIRECTION_VALID 0x02
#define WEATHER_STATUS_VANE_FAULT 0x04     // latest vane reading was an open circuit or a short
#define WEATHER_STATUS_WATCHDOG_RESET 0x08 // last reset was the watchdog
#define WEATHER_STATUS_RAIN_VALID 0x10     // a full rain hour is in; until then rain reads 0

struct __attribute__((packed)) WeatherRegisters {
  uint8_t device_id;            // 0x00  WEATHER_DEVICE_ID
  uint8_t map_version;          // 0x01  WEATHER_MAP_VERSION
  uint8_t status;               // 0x02  WEATHER_STATUS_* bits
  uint8_t sequence;             // 0x03  increments on every 1 s update
  uint16_t wind_avg;            // 0x04  mean speed over the window, 0.01 m/s
  uint16_t wind_gust;           // 0x06  highest rolling 3 s mean in the window, 0.01 m/s
  uint16_t wind_direction;      // 0x08  speed-weighted mean heading, 0.1 degree (0-3599)
  uint16_t rain_last_hour;      // 0x0A  rainfall over the last 60 complete minutes, 0.01 mm
  uint32_t uptime_seconds;      // 0x0C
};

static_assert(sizeof(WeatherRegisters) == 0x10, "register map layout changed");
