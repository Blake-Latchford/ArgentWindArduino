# Board support: to do

Goal: one build for the boards in [the README](../README.md#supported-boards): the SparkFun Qwiic Pro
Micro USB-C (primary) and the 3.3V/8MHz Pro Micro.

## 1. Verification

- [ ] RAK reads `0x57` at 0x6E with the jumper.
- [ ] After a watchdog reset the bootloader checks a "magic key" in RAM to decide whether to stay in
  the bootloader. The core code that sets the key isn't built any more; confirm that none of the
  firmware's own RAM at the key's address can hold the key's value at a reset.
- [ ] Qwiic Pro Micro, with its jumper set to 3.3V: starts reliably from power-on and from reset. The
  bootloader runs at 16MHz, beyond the 32U4's rating at 3.3V.
- [ ] Qwiic Pro Micro: check its schematic for whether the RAK's 3.3V can be fed into VCC, back-feeding
  the AP2112.
- [ ] Qwiic Pro Micro: sleep current measured.