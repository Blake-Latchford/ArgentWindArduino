#include "WindSensor.h"

#include <math.h>

namespace {

constexpr uint16_t WIND_GUST_SECONDS = 3U;  // WMO gust: highest rolling 3-second mean

// sin and cos of each Heading, scaled by HEADING_SCALE, the full range of int16_t.
constexpr int32_t HEADING_SCALE = 32767;
constexpr int16_t HEADING_SIN[HEADING_COUNT] = {
  0, 12539, 23170, 30273, 32767, 30273, 23170, 12539,
  0, -12539, -23170, -30273, -32767, -30273, -23170, -12539,
};
constexpr int16_t HEADING_COS[HEADING_COUNT] = {
  32767, 30273, 23170, 12539, 0, -12539, -23170, -30273,
  -32767, -30273, -23170, -12539, 0, 12539, 23170, 30273,
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
    _weighted_sin += sign * (int32_t)sample.pulses * HEADING_SIN[i];
    _weighted_cos += sign * (int32_t)sample.pulses * HEADING_COS[i];
    _heading_count += sign;
    _unit_sin += sign * HEADING_SIN[i];
    _unit_cos += sign * HEADING_COS[i];
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

  // Walk the window oldest to newest, keeping a rolling sum of the last WIND_GUST_SECONDS entries.
  // Skip triples that cross head, since the readings are not continous there.
  uint16_t lead = _head;
  uint16_t rolling = 0;

  // Don't use the obvious algorithm because modulo is slow on AVR.
  // Fill the first WIND_GUST_SECONDS.
  for (uint16_t n = 0; n < WIND_GUST_SECONDS; n++) {
    rolling += _samples[lead].pulses;
    if (++lead >= WIND_WINDOW_SECONDS) lead = 0;
  }
  uint16_t best = rolling;

  auto slide = [&](uint16_t add, uint16_t drop) __attribute__((always_inline)) {
    rolling += _samples[add].pulses;
    rolling -= _samples[drop].pulses;
    if (rolling > best) {
      best = rolling;
    }
  };

  // Check triples right of head.
  if (lead > _head) {
    for (; lead < WIND_WINDOW_SECONDS; lead++)
      slide(lead, lead - WIND_GUST_SECONDS);
    lead = 0;
  }
  // Check valid triples spanning the wrap.
  for (; lead < WIND_GUST_SECONDS && lead < _head; lead++) {
    slide(lead, lead + WIND_WINDOW_SECONDS - WIND_GUST_SECONDS);
  }
  // Check triples left of the head.
  for (; lead < _head; lead++) slide(lead, lead - WIND_GUST_SECONDS);

  return hundredthsMetersPerSecond(best, WIND_GUST_SECONDS);
}

WindSensor::Direction WindSensor::direction() const {
  Direction result;
  if (!windowFull() || _heading_count == 0)
    return result;

  result.stddev = stddev();

  // In a dead calm every heading has zero weight, so there is no direction to report.
  if (_weighted_sin == 0 && _weighted_cos == 0) {
    return result;
  }
  // atan2 gives -pi..pi, so this is -32768..32768.
  double radians = atan2((double)_weighted_sin, (double)_weighted_cos);
  result.angle = (uint16_t)lround(radians * (32768.0 / M_PI));
  result.valid = true;
  return result;
}

// Needs at least one valid heading in the window.
uint16_t WindSensor::stddev() const {
  // Yamartino :
  // e = sqrt(1 - (sa^2 + ca^2))
  // sigma = asin(e) * (1 + (2/sqrt(3) - 1) * e^3)
  const double n = (double)_heading_count * HEADING_SCALE;
  const double sin_average = _unit_sin / n;
  const double cos_average = _unit_cos / n;
  const double epsilon_squared = 1.0 - (sin_average * sin_average + cos_average * cos_average);
  if (epsilon_squared <= 0.0)
    return 0;
  const double epsilon = sqrt(epsilon_squared);
  const double radians = asin(epsilon) * (1 + (2 / sqrt(3.0) - 1) * epsilon * epsilon * epsilon);
  return (uint16_t)lround(radians * (32768.0 / M_PI));
}
