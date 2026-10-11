# Board support refactor: to do

Goal: build the firmware for every board in [hardware.md](hardware.md#supported-boards) from one source
tree: Pro Mini, Uno R3, RedBoard Qwiic, Pro Micro (3.3V/8MHz and 5V/16MHz) and Qwiic Pro Micro.

## 1. PlatformIO environments

- [ ] Add `promicro` and `promicro_debug` to `default_envs` once they link (section 2).
- [ ] Check that the RedBoard Qwiic's bootloader runs at 115200, as the Uno's does (the `uno`
  environment's upload speed).

## 2. USB on the 32U4 (blocking: decide first)

The 32U4 builds won't link as things stand. The Arduino core's `USBCore.cpp` sets the USB PLL input
divider from `F_CPU` and has `#error "Clock rate of F_CPU not supported"` for anything other than 8 or
16MHz. The firmware sets `F_CPU` to 2MHz. This is the only compile error for the Pro Micro. The core's
`main()` also calls `USBDevice.attach()` before `setup()`, which would leave USB running.

- [ ] Choose an approach. Options:
  - **Build without the core's USB stack.** For example, a custom board definition or variant that
    doesn't compile USB, or our own `main()` without `USBDevice.attach()`. Cleanest at run time. Needs
    research into what PlatformIO and the core allow.
  - Keep `F_CPU` at a crystal frequency and add our own constant for the real 2MHz clock. That would
    affect `_delay_us()`, `Serial` baud rates and the `OneHzSleep` and `ClockPrescale` checks. It's
    fragile, it would bring back a per-board crystal setting, and USB would still be attached and then
    misbehave at 2MHz.
- [ ] Whatever the choice, make sure USB is detached and `power_usb_disable()` is called. Don't write
  `PLLCSR` from the sketch: SparkFun warns this can make the board look bricked.
- [ ] Confirm that upload by double reset works with USB off (see
  [Programming](hardware.md#programming)).

## 3. Remaining changes in `src/`

Keep the existing file layout: setup first, then periodic code, then ISRs. Chip differences (power
registers, ADC channels) stay in the driver that uses them, under `#ifdef` on the register or macro.

- [ ] **`ClockPrescale.cpp`**: test the crystal measurement on hardware, at 8MHz and 16MHz. It assumes
  the CPU starts at the crystal frequency: the `CKDIV8` fuse unprogrammed, and the bootloader leaving
  `CLKPR` at divide-by-1. That holds for the stock bootloaders as far as I know; confirm it, especially
  for the Pro Micro's.
- [ ] **`DisableUnusedHardware.cpp`**: per-chip power-reduction calls. `power_timer2_disable()` and
  `power_usart0_disable()` compile for the 32U4 in this avr-libc, but they set reserved `PRR0` bits. What the 32U4 needs is Timer3
  (`power_timer3_disable()`), Timer4 (no macro: set `PRTIM4` in `PRR1`), USART1 outside debug builds
  (`power_usart1_disable()`) and USB (section 2).
- [ ] **`IICServer.cpp`**: `Wire.begin()` turns the internal pull-ups on before we turn them off. On a
  5V board wired straight to a 3.3V bus (Qwiic Pro Micro), that briefly pulls the bus towards 5V
  through ~30k. Harmless in practice, but check whether the TWI can be enabled without it.
- [ ] **`Watchdog.cpp`**: test watchdog recovery on the 32U4 with its bootloader. The bootloader runs
  after a watchdog reset and uses RAM, so it may overwrite the `.noinit` marker, which would lose the
  watchdog-reset status bit. It also checks a "magic key" in RAM to decide whether to stay in the
  bootloader. Confirm the firmware can't leave that key set by accident.
- [ ] **`OneHzSleep.cpp`**: Timer1 is the same on both chips; check only.

## 4. Decisions

- [ ] **Qwiic Pro Micro I2C levels:** accept the out-of-spec direct Qwiic connection, or document
  removing R5/R7 and fitting a level shifter (see
  [its notes](hardware.md#sparkfun-qwiic-pro-micro-usb-c)).
- [ ] **Brown-out detection:** check each board's BOD fuse setting against its supply. Boards use their
  bootloader's fuses unless reflashed.

## 5. Verification, per board

- [ ] Builds without warnings; RAM and flash fit. The Pro Mini build uses 78% of the 328P's 2KB RAM.
  The 32U4 has 2.5KB, and 28KB flash because its bootloader takes 4KB.
- [ ] Debug build: exactly one line per second (this checks the crystal measurement picked the right
  divider), pulses count, `vane_raw` tracks the vane.
- [ ] Vane counts match the Pro Mini's for the same resistor (the reading is ratiometric).
- [ ] RAK reads `0x57` at 0x6C, and at 0x6E with the jumper.
- [ ] Watchdog reset sets the status bit (hang the loop on purpose in a test build).
- [ ] Sleep current measured on the battery-capable boards (Pro Mini, Pro Micro 3.3V).

## 6. Docs

- [ ] README: the intro, the rest of "How it works", the I2C section and the calibration steps still
  describe only the Pro Mini. Generalize them.
- [ ] README and hardware.md: remove the "doesn't build yet" notes for the Pro Micro once it builds.
