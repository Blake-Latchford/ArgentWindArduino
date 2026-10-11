- **Clock and sleep:** before `main()`, the chip times its crystal against the watchdog oscillator and
  divides it to 2MHz (8MHz by 4, 16MHz by 8), so `F_CPU` is 2MHz throughout. Between interrupts it sits
  in idle sleep. Timer0 is off, so `millis()` and `delay()` don't work. SPI, Timers 3 and 4, USB, the comparator and (outside debug builds) the USART are off too. The ADC is powered
  only while the vane is being sampled.
- **Time base:** Timer1 in CTC mode interrupts at 1Hz.
- **Hang recovery:** the watchdog runs in interrupt-then-reset mode with a 2s timeout. The main loop
  re-arms it on each tick. If the watchdog does reset the chip, the next boot sets a status bit.
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