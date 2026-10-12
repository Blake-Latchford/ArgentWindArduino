#pragma once

#include <stdint.h>

// The Argent Data tipping-bucket rain gauge: one reed switch closure per 0.2794 mm (0.011") of rain. Takes
// the tip count once per second and gives out rainfall in 0.01 mm over a rolling hour: the last 60
// complete minutes, advancing each minute.

#define RAIN_HOUR_MINUTES 60

class RainGauge {
public:
  // Record one second's bucket tips.
  void addSample(uint8_t tips);

  // Rain is only reported over a full hour. Until then it reads 0.
  bool hourFull() const { return _filled_minutes == RAIN_HOUR_MINUTES; }

  // Rainfall over the last 60 complete minutes, or 0 until the hour is full.
  uint16_t lastHourRain() const;

private:
  uint8_t _current_tips = 0;   // tips in the minute under way, not yet in the hour
  uint8_t _second = 0;         // seconds into the minute under way

  uint8_t _minute_tips[RAIN_HOUR_MINUTES] = {};   // tips per complete minute
  uint8_t _minute = 0;                            // next slot to write
  uint8_t _filled_minutes = 0;
  uint16_t _hour_tips = 0;                        // running sum of _minute_tips
};
