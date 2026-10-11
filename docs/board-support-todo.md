# Board support refactor: to do

Goal: build the firmware for every board in [hardware.md](hardware.md#supported-boards) from one source
tree: Pro Mini, Uno R3, RedBoard Qwiic, Pro Micro (3.3V/8MHz and 5V/16MHz) and Qwiic Pro Micro.

Today `src/` assumes an ATmega328P with an 8MHz crystal, and names registers and port bits directly.
`lib/WeatherCore` has no hardware dependencies and needs no changes.

The boards split into two chips:
- **ATmega328P** (Pro Mini, Uno, RedBoard): same pins as now. Only the crystal differs (8 or 16MHz).
- **ATmega32U4** (Pro Micros): I2C moves to D2/D3, the reed switches to D8/D9 (PORTB), A0 is ADC
  channel 7, there's no Timer2 or USART0, and the USB core has to be dealt with.

## 1. Board definition header

- [ ] Add `src/Board.h`: everything that differs per board, in one place. Select it with
  `WEATHER_BOARD_*` defines from `platformio.ini`, and check the chip against the board with
  `__AVR_ATmega328P__` / `__AVR_ATmega32U4__`:
  - crystal frequency (`WEATHER_CRYSTAL_HZ`), which sets the clock prescale
  - reed switch pins (Arduino pin numbers)
  - vane drive pin and vane ADC pin
  - address jumper pins (D5, D6 on every board)
  - LED pins and the level that turns each one off (Pro Micro RX/TX LEDs are off when **high**)
  - unused pins to park, including pins not brought out (32U4: PB7, PC7, PD6, PE2, PF0, PF1)
  - debug serial port (`Serial` on the 328P, `Serial1` on the 32U4)
- [ ] Where the core already gives the mapping, derive registers from Arduino pin numbers instead of
  hard-coding them: `digitalPinToPCICR`, `digitalPinToPCICRbit`, `digitalPinToPCMSK`,
  `digitalPinToPCMSKbit`, `digitalPinToBitMask`, `portInputRegister`, `analogPinToChannel` (32U4),
  and `SDA`/`SCL`. All exist in both the `standard` and `sparkfun_promicro` variants.

## 2. PlatformIO environments

- [ ] Add environments, each with `board_build.f_cpu = 2000000L` and its `WEATHER_BOARD_*` and
  `WEATHER_CRYSTAL_HZ` defines:

  | Env | `board` | Crystal | Notes |
  |---|---|---|---|
  | `promini` | `pro8MHzatmega328` | 8MHz | Existing |
  | `uno` | `uno` | 16MHz | Optiboot, 115200 upload |
  | `redboard_qwiic` | `sparkfun_redboard` | 16MHz | Check the upload speed matches the RedBoard Qwiic's bootloader |
  | `promicro8` | `sparkfun_promicro8` | 8MHz | |
  | `promicro16` | `sparkfun_promicro16` | 16MHz | Also the Qwiic Pro Micro |

- [ ] A `_debug` variant of each, as `promini_debug` does now. Shared flags go in a common `[env]` or
  `[avr]` section.
- [ ] Build every environment in CI (or a `pio run` with all of them listed in `default_envs`), so a
  change for one board can't silently break another.

## 3. USB on the 32U4 (blocking: decide first)

The 32U4 builds won't compile as things stand. The Arduino core's `USBCore.cpp` sets the USB PLL input
divider from `F_CPU` and has `#error "Clock rate of F_CPU not supported"` for anything other than 8 or
16MHz. The firmware sets `F_CPU` to 2MHz. The core's `main()` also calls `USBDevice.attach()` before
`setup()`, which would leave USB running.

- [ ] Choose an approach. Options:
  - **Build without the core's USB stack.** For example, a custom board definition or variant that
    doesn't compile USB, or our own `main()` without `USBDevice.attach()`. Cleanest at run time. Needs
    research into what PlatformIO and the core allow.
  - Keep `F_CPU` at the crystal frequency and add our own constant for the real 2MHz clock. That
    would affect `_delay_us()`, `Serial` baud rates and the `OneHzSleep` and `ClockPrescale` checks.
    It's fragile, and USB would still be attached and then misbehave at 2MHz.
- [ ] Whatever the choice, make sure USB is detached and `power_usb_disable()` is called. Don't write
  `PLLCSR` from the sketch: SparkFun warns this can make the board look bricked.
- [ ] Confirm that upload by double reset works with USB off (see
  [Programming](hardware.md#programming)).

## 4. Per-file changes in `src/`

- [ ] **`ClockPrescale.cpp`**: take the divider from `WEATHER_CRYSTAL_HZ` (8MHz → `clock_div_4`,
  16MHz → `clock_div_8`). Fix the `#error` and comments that say "8MHz".
- [ ] **`DisableUnusedHardware.cpp`**: per-chip power-reduction calls. The 328P has Timer2 and USART0;
  the 32U4 has Timer3, Timer4, USART1 and USB instead (`power_timer2_disable()` doesn't exist there).
  Park the board's unused pins from `Board.h`, and turn its LEDs off at the right level.
- [ ] **`ReedSwitches.cpp`**: replace the hard-coded `PORTD`/`PCINT2_vect`/`PCMSK2` with the board's
  pins. Both switches must stay on one port so one vector covers them: PORTD (`PCINT2_vect`) on the
  328P, PORTB (`PCINT0_vect`) on the 32U4. Add a `static_assert` or `#error` if a board's two pins
  don't share a port.
- [ ] **`VaneAdc.cpp`**: ADC channel and `DIDR0` bit from the board's vane pin. A0 is channel 0 on the
  328P and channel 7 on the 32U4 (`ADC7D`). Channels 8 and up on the 32U4 also need `MUX5` in
  `ADCSRB`; not needed for A0, but handle it or reject it.
- [ ] **`IICServer.cpp`**: turn off the SDA/SCL pull-ups with the `SDA`/`SCL` pins rather than
  `PORTC`/`PC4`/`PC5` (on the 32U4 they're PD1/PD0). Move the address jumper pins to `Board.h`.
  - `Wire.begin()` turns the internal pull-ups on before we turn them off. On a 5V board wired
    straight to a 3.3V bus (Qwiic Pro Micro), that briefly pulls the bus towards 5V through ~30k.
    Harmless in practice, but check whether the TWI can be enabled without it.
- [ ] **`main.cpp`**: debug output through the board's debug port (`Serial1` on the 32U4).
- [ ] **`Watchdog.cpp`**: test watchdog recovery on the 32U4 with its bootloader. The bootloader runs
  after a watchdog reset and uses RAM, so it may overwrite the `.noinit` marker, which would lose the
  watchdog-reset status bit. It also checks a "magic key" in RAM to decide whether to stay in the
  bootloader. Confirm the firmware can't leave that key set by accident.
- [ ] **`OneHzSleep.cpp`**: Timer1 is the same on both chips; check only.
- [ ] Keep the existing file layout: setup first, then periodic code, then ISRs. Put the
  board-specific parts in `Board.h`, not in `#if` blocks scattered through each file.

## 5. Decisions

- [ ] **One reed switch pinout for all boards?** D8/D9 are on PORTB on both chips: PB0/PB1 on the
  328P, PB4/PB5 on the 32U4. Moving every board to D8/D9 would give one wiring diagram and simpler
  code. The cost is re-wiring any Pro Mini already built on D2/D3. The docs currently keep D2/D3 on
  the 328P boards.
- [ ] **Qwiic Pro Micro I2C levels:** accept the out-of-spec direct Qwiic connection, or document
  removing R5/R7 and fitting a level shifter (see
  [its notes](hardware.md#sparkfun-qwiic-pro-micro-usb-c)).
- [ ] **Brown-out detection:** check each board's BOD fuse setting against its supply. Boards use their
  bootloader's fuses unless reflashed.

## 6. Verification, per board

- [ ] Builds without warnings; RAM and flash fit. The Pro Mini build already uses 79% of the 328P's
  2KB RAM. The 32U4 has 2.5KB, and 28KB flash because its bootloader takes 4KB.
- [ ] Debug build: one line per second, pulses count, `vane_raw` tracks the vane.
- [ ] Vane counts match the Pro Mini's for the same resistor (the reading is ratiometric).
- [ ] RAK reads `0x57` at 0x6C, and at 0x6E with the jumper.
- [ ] Watchdog reset sets the status bit (hang the loop on purpose in a test build).
- [ ] Sleep current measured on the battery-capable boards (Pro Mini, Pro Micro 3.3V).

## 7. Docs

- [ ] README: the intro, layout table and "How it works" still say Pro Mini and 8MHz throughout.
  Generalize, and list the build environments.
- [ ] hardware.md: remove the firmware-status note once the boards build, and add the debug-serial
  details for the Pro Micros.
