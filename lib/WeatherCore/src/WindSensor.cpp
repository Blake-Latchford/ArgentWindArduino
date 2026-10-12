#include "WindSensor.h"

#include <math.h>

namespace {

// sin and cos of each Heading, scaled by HEADING_SCALE.
const int32_t HEADING_SCALE = 1000;
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

void WindSensor::addSample(WindSample newest) {
  if (windowFull()) {
    // The window is full, so the slot at _head holds the oldest sample, which drops out.
    removeFromSums(_samples[_head]);
  } else {
    _filled++;
  }

  _samples[_head] = newest;
  addToSums(newest);

  _head = (_head + 1) % WIND_WINDOW_SECONDS;
}

// `sign` is 1 to add `sample` to the running sums, -1 to remove it.
void WindSensor::changeSums(const WindSample& sample, int8_t sign) {
  _pulse_sum += sign * (int32_t)sample.pulses;
  if (sample.heading != Heading::Invalid) {
    uint8_t i = (uint8_t)sample.heading;
    _heading_pulse_sum += sign * (int32_t)sample.pulses;
    _weighted_sin += sign * (int32_t)sample.pulses * HEADING_SIN[i];
    _weighted_cos += sign * (int32_t)sample.pulses * HEADING_COS[i];
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
  // full window, the oldest sample is the one _head is about to overwrite.
  uint16_t rolling = 0;
  uint16_t best = 0;
  for (uint16_t n = 0; n < WIND_WINDOW_SECONDS; n++) {
    uint16_t i = (_head + n) % WIND_WINDOW_SECONDS;
    rolling += _samples[i].pulses;
    if (n >= WIND_GUST_SECONDS) {
      rolling -= _samples[(i + WIND_WINDOW_SECONDS - WIND_GUST_SECONDS) % WIND_WINDOW_SECONDS].pulses;
    }
    if (n + 1 >= WIND_GUST_SECONDS && rolling > best) {
      best = rolling;
    }
  }
  return hundredthsMetersPerSecond(best, WIND_GUST_SECONDS);
}

WindSensor::Direction WindSensor::direction() const {
  Direction result;
  if (!windowFull())
    return result;

  if (averageSpeed() < 5)
    return result;

  // In a dead calm every heading has zero weight, so there is no direction to report.
  if (_weighted_sin == 0 && _weighted_cos == 0) {
    return result;
  }

  // atan2 gives -pi..pi, so this is -32768..32768. Converting to uint16_t wraps it modulo a full turn:
  // west of north comes out as 0x8000..0xFFFF, and both ends of the range land on south (0x8000).
  double radians = atan2((double)_weighted_sin, (double)_weighted_cos);
  result.angle = (uint16_t)(int32_t)lround(radians * (32768.0 / M_PI));
  result.variable = variable();
  result.valid = true;
  return result;
}

bool WindSensor::variable() const {
  // Each pulse adds a vector HEADING_SCALE long, so the threshold is a length too. Comparing squared
  // lengths avoids the square root. The worst case, 255 pulses every second, squares to about 1e16, so
  // the squares need 64 bits.
  int64_t threshold = (int64_t)_heading_pulse_sum * HEADING_SCALE * WIND_VARIABLE_RESULTANT_PER_MILLE / 1000;
  int64_t s = _weighted_sin;
  int64_t c = _weighted_cos;
  return ((s * s) + (c * c)) < (threshold * threshold);
}
