# Board support: to do

Goal: one build for the boards in [the README](../README.md#supported-boards): the SparkFun Qwiic Pro
Micro USB-C (primary) and the 3.3V/8MHz Pro Micro.

## 1. Verification, per board

- [ ] Debug build: exactly one line per second, pulses count, `vane_raw` tracks the vane. One line per
  second checks the crystal measurement (`ClockPrescale.cpp`) picked the right divider, at 8MHz (Pro
  Micro) and 16MHz (Qwiic Pro Micro), and the 1Hz Timer1 tick (`OneHzSleep.cpp`).
- [ ] The crystal measurement assumes the CPU starts at the crystal frequency: the `CKDIV8` fuse
  unprogrammed, and the bootloader leaving `CLKPR` at divide-by-1. That holds for the stock bootloaders
  as far as I know; confirm it for the Caterina bootloader on both boards.
- [ ] Vane counts match between the two boards for the same resistor (the reading is ratiometric).
- [ ] RAK reads `0x57` at 0x6C, and at 0x6E with the jumper.
- [ ] Watchdog reset sets the status bit (hang the loop on purpose in a test build). The bootloader
  runs after a watchdog reset and uses RAM, so it may overwrite the `.noinit` marker and lose the bit.
- [ ] After a watchdog reset the bootloader checks a "magic key" in RAM to decide whether to stay in
  the bootloader. The core code that sets the key isn't built any more; confirm that none of the
  firmware's own RAM at the key's address can hold the key's value at a reset.
- [ ] Qwiic Pro Micro, with its jumper set to 3.3V: starts reliably from power-on and from reset. The
  bootloader runs at 16MHz, beyond the 32U4's rating at 3.3V.
- [ ] Qwiic Pro Micro: check its schematic for whether the RAK's 3.3V can be fed into VCC, back-feeding
  the AP2112.
- [ ] Brown-out detection: check each board's BOD fuse setting against its supply. Boards use their
  bootloader's fuses unless reflashed.
- [ ] Sleep current measured on both boards.
