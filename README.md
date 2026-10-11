# ArgentWindArduino

Firmware for an Arduino Pro Mini (3.3V, 8MHz) that reads an Argent Data wind and rain sensor assembly and
serves finished measurements to a RAK WisBlock over I2C: mean wind speed, gust and direction over 10
minutes, and rainfall in millimetres. All averaging happens on the Pro Mini, so the reader never sees pulse
counts or raw ADC readings.

Wiring, parts and cabling are in [docs/hardware.md](docs/hardware.md).

## Layout

| Path | What |
|---|---|
| `src/main.cpp` | Brings up the hardware; once a second, feeds the readings to `WeatherStation` and updates the served registers |
| `src/Board.h` | Pin map (the same on every board), and the LEDs and debug port for each chip |
| `src/NoCoreUsb.h` | Leaves the Arduino core's USB stack out of the 32U4 builds |
| `src/ClockPrescale.cpp` | Watchdog off; measures the crystal and prescales it to 2MHz, before `main()` |
| `src/DisableUnusedHardware.*` | Unused peripherals and pins off |
| `src/OneHzSleep.*` | 1Hz Timer1 interrupt; idle sleep between seconds |
| `src/Watchdog.*` | Interrupt-then-reset watchdog |
| `src/ReedSwitches.*` | Anemometer and rain gauge pulse counting |
| `src/VaneAdc.*` | Powers the vane divider and ADC for each sample |
| `src/IICServer.*` | I2C slave serving the register map |
| `lib/WeatherCore/WindSensor.*` | Anemometer + vane: 10-minute ring buffers; mean speed, gust, direction |
| `lib/WeatherCore/RainGauge.*` | Rolling last-hour rainfall |
| `lib/WeatherCore/VaneDecoder.*` | Vane ADC counts → 16 headings |
| `lib/WeatherCore/WeatherStation.*` | Both sensors together; builds the register snapshot |
| `lib/WeatherCore/WeatherRegisters.h` | I2C register map, shared with the reading side |
| `test/` | Host-side unit tests for `lib/WeatherCore` |

`src/` is AVR-specific. `lib/WeatherCore` has no hardware dependencies and builds on any target.

## Build, test, flash

This project uses [PlatformIO](https://platformio.org/).

There's one firmware per chip. The environments differ only in how they upload:

| Env | Boards | Upload |
|---|---|---|
| `promini` | Pro Mini | 3.3V FTDI adapter. Set `upload_speed` to the baud rate your Optiboot build uses |
| `uno` | Uno R3, RedBoard Qwiic | USB |
| `promicro` | Pro Micro 3.3V and 5V, Qwiic Pro Micro | USB. Double-tap reset, then upload within 8 seconds (see [Programming](docs/hardware.md#programming)) |

```sh
pio test -e native              # unit tests on the host
pio run                         # build every environment
pio run -e uno -t upload        # build and flash one board
```

Each environment has a `_debug` variant, such as `promini_debug`, that prints each second's readings on
the debug serial port at 250000 baud (`pio device monitor -e promini_debug`).

## How it works

- **Clock and sleep:** before `main()`, the chip times its crystal against the watchdog oscillator and
  divides it to 2MHz (8MHz by 4, 16MHz by 8), so `F_CPU` is 2MHz throughout. Between interrupts it sits
  in idle sleep. Timer0 is off, so `millis()` and `delay()` don't work. SPI, Timer2, the comparator and (outside debug builds) the USART are off too. The ADC is powered
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

## I2C register map

The Pro Mini is an I2C slave at address `0x6C` (`0x6E` with a jumper across D5 and D6; see
[docs/hardware.md](docs/hardware.md#i2c-bus)), running at 100kHz. To read, write one byte to set the
register pointer, then read. Each read returns bytes from the pointer to the end of the map, all from one
snapshot, so multi-byte values are never torn. Values are little-endian.

| Offset | Size | Name | Units / meaning |
|---|---|---|---|
| 0x00 | 1 | `device_id` | `0x57` |
| 0x01 | 1 | `map_version` | `3` |
| 0x02 | 1 | `status` | bit 0 wind valid, bit 1 direction valid, bit 2 vane fault (latest sample), bit 3 last reset was watchdog, bit 4 rain valid |
| 0x03 | 1 | `sequence` | increments every second |
| 0x04 | 2 | `wind_avg` | 0.01 m/s, valid when status bit 0 is set |
| 0x06 | 2 | `wind_gust` | 0.01 m/s, valid when status bit 0 is set |
| 0x08 | 2 | `wind_direction` | 0.1°, 0–3599, valid when status bit 1 is set |
| 0x0A | 2 | `rain_last_hour` | rainfall over the last 60 complete minutes, 0.01 mm, valid when status bit 4 is set |
| 0x0C | 4 | `uptime_seconds` | |

To read everything, write `0x00` and read 16 bytes.

## Calibrating the vane

The table in `lib/WeatherCore/VaneDecoder.cpp` uses nominal datasheet resistances. To replace it with real
values:

1. Install the vane on its real cable, and flash the `promini_debug` build.
2. Point the vane at each of the 16 headings in turn.
3. At each heading, record the `vane_raw` count from the serial output.
4. Put the recorded counts into `VANE_COUNTS`.
5. If a measured count lands outside the open/short thresholds, adjust those too.

## Bench checks

- With the debug build, spin the anemometer and confirm that `pulses` follows it.
- After the first hour (status bit 4 set), tip the rain bucket N times. Within two minutes, confirm that
  `rain_last_hour` goes up by N × 27.94 (0.01 mm).
- Check the reed-switch edges on a scope with the real cables, and adjust the RC values if needed.
