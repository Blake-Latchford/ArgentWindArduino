#include "WindSensor.h"

#include <math.h>

namespace {

// sin and cos of each Heading, scaled by 1000.
const int16_t HEADING_SIN[HEADING_COUNT] = {
  0, 383, 707, 924, 1000, 924, 707, 383, 0, -383, -707, -924, -1000, -924, -707, -383,
};
const int16_t HEADING_COS[HEADING_COUNT] = {
  1000, 924, 707, 383, 0, -383, -707, -924, -1000, -924, -707, -383, 0, 383, 707, 924,
};

// 0.01 m/s for `pulses` over `seconds`, at 2/3 m/s per pulse per second, rounded.
uint16_t hundredthsMetersPerSecond(uint32_t pulses, uint32_t seconds) {
  uint32_t divisor = 3 * seconds;
  return (uint16_t)((pulses * 200 + divisor / 2) / divisor);
}

}  // namespace

void WindSensor::addSecond(uint8_t pulses, uint16_t vane_counts) {
  Second newest = {pulses, VaneDecoder::decode(vane_counts)};
  _vane_fault = newest.heading == Heading::Invalid;

  if (windowFull()) {
    // The window is full, so the slot at _head holds the oldest second, which drops out.
    removeFromSums(_seconds[_head]);
  } else {
    _filled++;
  }

  _seconds[_head] = newest;
  addToSums(newest);

  _head = (_head + 1) % WIND_WINDOW_SECONDS;
}

// `sign` is 1 to add `second` to the running sums, -1 to remove it.
void WindSensor::changeSums(const Second& second, int8_t sign) {
  _pulse_sum += sign * (int32_t)second.pulses;
  if (second.heading != Heading::Invalid) {
    uint8_t i = (uint8_t)second.heading;
    _weighted_sin += sign * (int32_t)second.pulses * HEADING_SIN[i];
    _weighted_cos += sign * (int32_t)second.pulses * HEADING_COS[i];
  }
}

uint16_t WindSensor::averageSpeed() const {
  if (!windowFull()) {
    return 0;
  }
  return hundredthsMetersPerSecond(_pulse_sum, WIND_WINDOW_SECONDS);
}

uint16_t WindSensor::gustSpeed() const {
  if (!windowFull()) {
    return 0;
  }

  // Walk the window oldest to newest, keeping a rolling sum of the last WIND_GUST_SECONDS entries. In a
  // full window, the oldest second is the one _head is about to overwrite.
  uint16_t rolling = 0;
  uint16_t best = 0;
  for (uint16_t n = 0; n < WIND_WINDOW_SECONDS; n++) {
    uint16_t i = (_head + n) % WIND_WINDOW_SECONDS;
    rolling += _seconds[i].pulses;
    if (n >= WIND_GUST_SECONDS) {
      rolling -= _seconds[(i + WIND_WINDOW_SECONDS - WIND_GUST_SECONDS) % WIND_WINDOW_SECONDS].pulses;
    }
    if (n + 1 >= WIND_GUST_SECONDS && rolling > best) {
      best = rolling;
    }
  }
  return hundredthsMetersPerSecond(best, WIND_GUST_SECONDS);
}

bool WindSensor::direction(uint16_t& tenths) const {
  if (!windowFull()) {
    return false;
  }

  // In a dead calm every heading has zero weight, so there is no direction to report.
  if (_weighted_sin == 0 && _weighted_cos == 0) {
    return false;
  }

  double degrees = atan2((double)_weighted_sin, (double)_weighted_cos) * (180.0 / M_PI);
  if (degrees < 0) {
    degrees += 360.0;
  }
  uint16_t result = (uint16_t)lround(degrees * 10.0);
  tenths = result >= 3600 ? result - 3600 : result;
  return true;
}
