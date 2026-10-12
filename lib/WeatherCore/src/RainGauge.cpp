#include "RainGauge.h"

// Rain in 0.01 mm for `tips` bucket tips at 0.2794 mm each, rounded. Converting from the integer tip count
// keeps rounding error from accumulating.
static uint32_t hundredthsMillimeters(uint32_t tips) { return (tips * 2794 + 50) / 100; }

void RainGauge::addSample(uint8_t tips) {
  // 255 tips in a minute is about 71 mm, over twice the world record, so only a faulty switch gets here.
  // Saturate rather than wrap.
  if (tips > 0xFF - _current_tips) {
    _current_tips = 0xFF;
  } else {
    _current_tips += tips;
  }
  if (++_second < 60) {
    return;
  }

  // The minute is complete: it joins the hour.
  if (hourFull()) {
    // The hour is full, so the slot at _minute holds the oldest minute, which drops out.
    _hour_tips -= _minute_tips[_minute];
  } else {
    _filled_minutes++;
  }
  _minute_tips[_minute] = _current_tips;
  _hour_tips += _current_tips;
  _minute = (_minute + 1) % RAIN_HOUR_MINUTES;

  _current_tips = 0;
  _second = 0;
}

uint16_t RainGauge::lastHourRain() const {
  if (!hourFull()) {
    return 0;
  }
  uint32_t rain = hundredthsMillimeters(_hour_tips);
  return rain > 0xFFFF ? 0xFFFF : (uint16_t)rain;
}
