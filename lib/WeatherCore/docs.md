# WeatherCore

Turns one second of raw sensor readings at a time into the reported values. It has no hardware
dependencies, and builds and tests on the host (`pio test -e native`).

## What a board provides

Once a second, from the main loop, a board calls `WeatherStation::add()` with a `WeatherSample`
(`WeatherStation.h`), then reads `WeatherStation::currentRegisters()` and sends them on, over I2C or any
other transport. The sample holds:

- **Seconds** since the previous sample, from an accurate clock. Wind speed is pulses per second, so a
  timing error is a speed error.
- **Anemometer and rain gauge closures** since the previous sample. Count each switch's debounced
  falling edges in an interrupt with a `PulseCounter` (`PulseCounter.h`), and `take()` from the main loop.
- **One 10-bit vane reading**, 0-1023, so set a higher-resolution ADC to 10 bits. The vane is the lower
  leg of a divider under 10k, driven from the ADC's reference voltage (`VaneDecoder.h`).

`WeatherStation` isn't interrupt-safe: only `PulseCounter::increment()` runs in an interrupt.

## Reported values

- **Wind speed:** 1 pulse/s = 2.4 km/h = 2/3 m/s. The mean covers the full 600 s window. Until the
  window has filled (the first 10 minutes after power-up), the wind-valid status bit is clear, speed and
  gust read 0, and direction is invalid.
- **Gust:** the highest rolling 3-second mean in the window, per the WMO definition. It is not limited to
  fixed buckets.
- **Direction:** the vector mean of the vane headings, each weighted by that second's pulse count. In a
  dead calm (no pulses in the window) there is no direction, and the direction-valid status bit is
  clear. Seconds where the vane reads open (the magnet between two reeds) are skipped.
- **Rain:** 0.2794 mm per bucket tip. Last-hour rainfall covers the last 60 complete minutes and
  advances each minute; until the first hour has passed, the rain-valid status bit is clear and it reads
  0. Tips are kept as counts in one-minute buckets and converted from the integer count, so rounding
  doesn't accumulate.
- **Rolling windows:** every read returns each value over its full standard window (10 minutes for
  wind, one hour for rain) ending at the latest update, so reads a minute apart overlap. For
  non-overlapping periods, read once per period.