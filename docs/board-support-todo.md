# Board support refactor: to do

Goal: build the firmware for every board in [hardware.md](hardware.md#supported-boards) from one source
tree: Pro Mini, Uno R3, RedBoard Qwiic, Pro Micro (3.3V/8MHz and 5V/16MHz) and Qwiic Pro Micro.

## 1. Remaining changes in `src/`

Keep the existing file layout: setup first, then periodic code, then ISRs. Chip differences (power
registers, ADC channels) stay in the driver that uses them, under `#ifdef` on the register or macro.

- [ ] **`ClockPrescale.cpp`**: test the crystal measurement on hardware, at 8MHz and 16MHz. It assumes
  the CPU starts at the crystal frequency: the `CKDIV8` fuse unprogrammed, and the bootloader leaving
  `CLKPR` at divide-by-1. That holds for the stock bootloaders as far as I know; confirm it, especially
  for the Pro Micro's.
- [ ] **`DisableUnusedHardware.cpp`**: per-chip power-reduction calls. `power_timer2_disable()` and
  `power_usart0_disable()` compile for the 32U4 in this avr-libc, but they set reserved `PRR0` bits.
  What the 32U4 needs is Timer3 (`power_timer3_disable()`), Timer4 (no macro: set `PRTIM4` in `PRR1`),
  USART1 outside debug builds (`power_usart1_disable()`).
- [ ] **`IICServer.cpp`**: `Wire.begin()` turns the internal pull-ups on before we turn them off. On a
  5V board wired straight to a 3.3V bus (Qwiic Pro Micro), that briefly pulls the bus towards 5V
  through ~30k. Harmless in practice, but check whether the TWI can be enabled without it.
- [ ] **`Watchdog.cpp`**: test watchdog recovery on the 32U4 with its bootloader. The bootloader runs
  after a watchdog reset and uses RAM, so it may overwrite the `.noinit` marker, which would lose the
  watchdog-reset status bit. It also checks a "magic key" in RAM to decide whether to stay in the
  bootloader. The core code that sets the key isn't built any more; confirm that none of the
  firmware's own RAM at the key's address can hold the key's value at a reset.
- [ ] **`OneHzSleep.cpp`**: Timer1 is the same on both chips; check only.

## 2. Decisions

- [ ] **Qwiic Pro Micro I2C levels:** accept the out-of-spec direct Qwiic connection, or document
  removing R5/R7 and fitting a level shifter (see
  [its notes](hardware.md#sparkfun-qwiic-pro-micro-usb-c)).
- [ ] **Brown-out detection:** check each board's BOD fuse setting against its supply. Boards use their
  bootloader's fuses unless reflashed.

## 3. Verification, per board

- [ ] Debug build: exactly one line per second (this checks the crystal measurement picked the right
  divider), pulses count, `vane_raw` tracks the vane.
- [ ] Vane counts match the Pro Mini's for the same resistor (the reading is ratiometric).
- [ ] RAK reads `0x57` at 0x6C, and at 0x6E with the jumper.
- [ ] Watchdog reset sets the status bit (hang the loop on purpose in a test build).
- [ ] Sleep current measured on the battery-capable boards (Pro Mini, Pro Micro 3.3V).

## 4. Docs

- [ ] README: the intro, the rest of "How it works", the I2C section and the calibration steps still
  describe only the Pro Mini. Generalize them.
