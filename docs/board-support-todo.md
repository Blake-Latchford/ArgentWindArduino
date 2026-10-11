# Board support refactor: to do

Goal: build the firmware for every board in [hardware.md](hardware.md#supported-boards) from one source
tree: Pro Mini, Uno R3, RedBoard Qwiic, Pro Micro (3.3V/8MHz and 5V/16MHz) and Qwiic Pro Micro.

## 1. Verification, per board

- [ ] Debug build: exactly one line per second, pulses count, `vane_raw` tracks the vane. One line per
  second checks the crystal measurement (`ClockPrescale.cpp`) picked the right divider, at 8MHz and
  16MHz, and the 1Hz Timer1 tick (`OneHzSleep.cpp`) on both chips.
- [ ] The crystal measurement assumes the CPU starts at the crystal frequency: the `CKDIV8` fuse
  unprogrammed, and the bootloader leaving `CLKPR` at divide-by-1. That holds for the stock bootloaders
  as far as I know; confirm it, especially for the Pro Micro's.
- [ ] Vane counts match the Pro Mini's for the same resistor (the reading is ratiometric).
- [ ] RAK reads `0x57` at 0x6C, and at 0x6E with the jumper.
- [ ] Watchdog reset sets the status bit (hang the loop on purpose in a test build). On the 32U4 the
  bootloader runs after a watchdog reset and uses RAM, so it may overwrite the `.noinit` marker and
  lose the bit.
- [ ] 32U4: after a watchdog reset the bootloader checks a "magic key" in RAM to decide whether to stay
  in the bootloader. The core code that sets the key isn't built any more; confirm that none of the
  firmware's own RAM at the key's address can hold the key's value at a reset.
- [ ] Qwiic Pro Micro, with its jumper set to 3.3V: starts reliably from power-on and from reset. The
  bootloader runs at 16MHz, beyond the 32U4's rating at 3.3V.
- [ ] Brown-out detection: check each board's BOD fuse setting against its supply. Boards use their
  bootloader's fuses unless reflashed.
- [ ] Sleep current measured on the battery-capable boards (Pro Mini, Pro Micro 3.3V).

## 2. Docs

- [ ] README: the intro, the rest of "How it works", the I2C section and the calibration steps still
  describe only the Pro Mini. Generalize them.
