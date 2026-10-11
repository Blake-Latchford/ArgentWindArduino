# Hardware

The interface board reads the wind and rain sensors and serves the results to a RAK WisBlock over I2C.
It can be built on any of the boards below.

## Supported boards

| Board | MCU, clock | Logic | I2C link to the RAK (3.3V) | Power | Field use on battery |
|---|---|---|---|---|---|
| **Arduino Pro Mini 3.3V/8MHz** | ATmega328P, 8MHz | 3.3V | Direct | RAK 3.3V into VCC | **Yes.** The low-power reference build |
| Arduino Pro Micro 3.3V/8MHz | ATmega32U4, 8MHz | 3.3V | Direct | RAK 3.3V into VCC | Yes, with the power LED removed |
| Arduino Pro Micro 5V/16MHz | ATmega32U4, 16MHz | 5V | External level shifter | Own 5V supply | Marginal |
| SparkFun Qwiic Pro Micro USB-C | ATmega32U4, 16MHz | 5V | Qwiic port, no level shifter (see [notes](#sparkfun-qwiic-pro-micro-usb-c)) | Own 5V supply | Marginal |
| SparkFun RedBoard Qwiic | ATmega328P, 16MHz | 5V | Qwiic port, level shifted on the board | Own supply (USB or barrel jack) | No |
| Arduino Uno R3 | ATmega328P, 16MHz | 5V | External level shifter | Own supply (USB or barrel jack) | No |

- **Uno** means the Uno R3 (and its SMD version). The Uno R4 uses a Renesas RA4M1, not an AVR, and is
  not supported.
- **Field use:** the firmware sleeps between interrupts, but it can't turn off what else is on the
  board. The Uno and RedBoard keep a USB interface chip, regulators and a power LED running, which
  draw milliamps however the firmware sleeps. Use them for bench work or mains/USB-powered installs.
  The 5V Pro Micros sit in between: no separate USB chip, but a regulator and power LED, and they need
  a 5V supply of their own.
- **Don't use the 3.3V jumpers** on the RedBoard Qwiic or Qwiic Pro Micro. Both keep their 16MHz
  crystal, which is beyond the chips' rating at 3.3V. The firmware drops to 2MHz, but the bootloader
  and startup run at the full 16MHz first.

## Board notes

### Arduino Pro Mini 3.3V/8MHz

- Remove the power LED.
- Remove the onboard regulator, or bypass it by feeding VCC directly.
- Power from the RAK's **3.3V rail**, with a common ground.
- Optiboot is recommended: it takes 512 bytes rather than the stock bootloader's 2K.
- A4 (SDA) and A5 (SCL) are not on the side rows on most Pro Mini boards; solder header pins or wires
  to those pads.

### Arduino Pro Micro (3.3V/8MHz or 5V/16MHz)

- The voltage is marked on the back. The **3.3V/8MHz** version wires straight to the RAK's I2C bus and
  can run from the RAK's 3.3V rail into VCC. The **5V/16MHz** version needs a level shifter and its own
  5V supply (see [Power](#power)).
- I2C is on **D2 (SDA) and D3 (SCL)**, not A4/A5.
- Remove the red power LED for field use.
- There's no reset button. To reset, short RST to GND.

### SparkFun Qwiic Pro Micro USB-C

- ATmega32U4 at **5V/16MHz**, with an AP2112 3.3V regulator (6V maximum input on RAW).
- The Qwiic port is **not level shifted**. SDA and SCL run straight to the 32U4's D2/D3, with 2.2k
  pull-ups to the 3.3V rail (R5, R7 on the v2.0 schematic). There's no jumper to remove them.
  - The RAK is safe: the I2C pins only pull low, and the pull-ups go to 3.3V. (The firmware turns off
    the 32U4's internal pull-ups, which would otherwise pull to 5V.)
  - The 32U4 is out of spec: at 5V its TWI input-high threshold is 0.7 × 5V = 3.5V, above the 3.3V bus.
    It usually works on a short bus, but it isn't guaranteed. For a permanent install, either accept
    that and test it, or remove R5/R7 and fit a proper level shifter.
  - The 2.2k pull-ups are in parallel with any on the RAK side. With 4.7k on the RAK, that's about 1.5k
    combined, still within the I2C 3mA sink limit at 3.3V.
- The red power LED runs from the 3.3V rail through 1k, about 1.5mA. Remove it for field use.
- The RX and TX LEDs (PB0 and PD5) are wired from VCC to the pin, so they light when the pin is **low**.
  The firmware must drive those pins high to keep them off.

### SparkFun RedBoard Qwiic

- ATmega328P at 5V/16MHz: the same chip as the Pro Mini, so the same pins.
- The Qwiic port has logic level converters on the board, so the RAK connects through the Qwiic port
  at 3.3V without an extra shifter. The converters include their own pull-ups.
- A4 and A5 are tied to the Qwiic bus. Their jumpers disconnect the converters; leave them closed.
- Leave the I/O voltage jumper at 5V (see above).

### Arduino Uno R3

- ATmega328P at 5V/16MHz: the same chip and pins as the Pro Mini.
- I2C is on A4/A5, also brought out on the SDA/SCL header pins next to AREF. The Uno has no I2C
  pull-ups of its own.
- Connect it to the RAK through a **bidirectional I2C level shifter**: a BSS138 MOSFET or PCA9306
  breakout, with the low side on the RAK's 3.3V and the high side on the Uno's 5V. Avoid shifter boards
  with their own 3.3V regulator unless that regulator is what powers the RAK side.
- D13's LED is driven through a buffer, so the firmware holds D13 low to keep it off.

## Power

- The RAK supplies **3.3V only**. Never connect a 5V rail or a 5V signal to it.
- **3.3V boards** (Pro Mini, Pro Micro 3.3V): feed the RAK's 3.3V into VCC, bypassing the regulator,
  with a common ground.
- **5V boards** (Uno, RedBoard Qwiic, Pro Micro 5V, Qwiic Pro Micro): power them separately: USB, the
  barrel jack (Uno and RedBoard, 7–12V), or RAW (Qwiic Pro Micro up to 6V; original Pro Micro up to
  12V). Connect GND to the RAK's GND.
- **Qwiic cables carry 3.3V on the red wire**, from the board's own regulator. If the RAK is powered
  separately, leave the red wire unconnected (cut it, or use a cable without it), so the two 3.3V
  regulators aren't tied together. GND, SDA and SCL still connect.

## Pin assignments

Every board uses the same pin numbers, except for I2C, which the chip fixes. `src/Board.h` is the
source of truth; keep this table in step with it.

| Function | Pin | Notes |
|---|---|---|
| I2C SDA | A4 on the 328P boards (Uno: also the SDA header pin); D2 on the Pro Micros | Also on the Qwiic port where there is one. Slave address 0x6C, or 0x6E with the address jumper |
| I2C SCL | A5 on the 328P boards (Uno: also the SCL header pin); D3 on the Pro Micros | |
| Anemometer reed switch | D8 | Pin-change interrupt, falling edge |
| Rain gauge reed switch | D9 | Pin-change interrupt, falling edge |
| Top of vane divider | D4 | HIGH only while sampling |
| Vane divider midpoint | A0 | ADC input |
| Address jumper | D5, D6 | Shunt fitted: 0x6E. Read at reset |
| Debug serial | D1 (TX), D0 (RX) | Debug builds only. `Serial1` on the Pro Micros |
| Power | VCC / 5V | See [Power](#power) |
| GND | GND | RAK GND, sensor grounds |

Spare: D7 on every board, and D2/D3 on the 328P boards.

**On the header.** The Pro Mini and Pro Micro have the same pin order: one long edge runs TX, RX, two
pins (RST/GND or GND/GND), then D2–D9; the other runs RAW, GND, RST, VCC, A3–A0, then four more. So
everything except A0 is on one edge: D4, then the D5–D6 jumper, D7 spare, then the reed switches on
D8–D9 at the end. On the Uno and RedBoard, D2–D7 are on the 8-pin digital header and D8/D9 start the
10-pin one.

**Why these pins:**
- The reed switches need pin-change interrupts on one port. The 32U4 has them only on PORTB, and D8/D9
  are on PORTB on both chips, so both switches share one interrupt everywhere. (D2/D3 are the 32U4's
  I2C pins.)
- The vane drive can be any digital output and the vane input any analog input that isn't I2C. D4 and
  A0 are on every board.
- The jumper needs two adjacent free pins: no free pin sits next to a GND on any of these boards, so D6
  is driven low to act as the jumper's ground while the firmware reads it at reset.
- On the Uno and RedBoard, D0/D1 also go to the USB-serial chip, so the debug output appears on the USB
  port. On the Pro Micros, `Serial` is USB; debug output goes to `Serial1` on the TX/RX pins and needs a
  USB-serial adapter at the board's logic voltage.

## I2C bus

- Exactly one set of pull-ups on the 3.3V side of the bus, sized for it. If the RAK base already has
  them, add none; otherwise 4.7k from SDA and from SCL to 3.3V. The RedBoard Qwiic's converters and the
  Qwiic Pro Micro's 2.2k resistors count as pull-ups.
- On 5V boards, the 3.3V side is the RAK's: either a level shifter between them (Uno, Pro Micro 5V),
  the RedBoard's onboard converters, or the Qwiic Pro Micro's direct connection (see its notes).
- The firmware turns off the internal pull-ups that `Wire.begin()` enables. To keep them on instead (a
  short bus with no external pull-ups, on a 3.3V board only), build with
  `-DWEATHER_I2C_EXTERNAL_PULLUPS=0` in `platformio.ini`. Never do that on a 5V board wired straight to
  the RAK: the internal pull-ups go to 5V.
- The slave address is 0x6C. A 2-pin jumper shunt across D5 and D6 changes it to 0x6E, for a second
  unit or a clash with another device on the bus. The firmware reads the jumper only at reset. To use
  a different address with the jumper off, add `-DWEATHER_I2C_ADDRESS=0x..` to `build_flags` in
  `platformio.ini`.
- Run the bus at 100kHz, not 400kHz. The firmware runs the CPU at 2MHz on every board, and the TWI
  slave needs a CPU clock of at least 16x SCL.

## Reed switch inputs (anemometer and rain, identical)

```
VCC ──[100k]──┬──────────── reed pin (see Pin assignments)
              │
            [22nF]
              │
GND ──────────┴── switch ──[1k]── (pin node)
```

- 100k pull-up from the pin to the board's VCC (3.3V or 5V): 33µA at 3.3V, 50µA at 5V, while the
  switch sits closed, against ~100µA with the internal pull-up.
- 22nF from the pin to GND debounces. With the 100k that's about a 2ms rise, fast enough for gust-level
  pulse rates of ~60Hz. The time constant is the same at either voltage.
- 1k in series between the switch cable and the pin node limits discharge current into the capacitor.
- The other side of each switch goes to GND; pulses are falling edges.
- The RC values are a starting point. Check them on a scope with the real cable.

## Wind vane

```
D4 ──[10k]──┬── A0
            │
          vane
            │
           GND
```

- The firmware sets D4 HIGH, reads A0, then sets D4 LOW, so the divider draws current only while
  sampling.
- The ADC reference is VCC, the same supply that drives the divider, so the reading is ratiometric: the
  same vane position gives the same count on a 3.3V or a 5V board, and supply drift cancels.
- The firmware maps readings to 16 headings using the table in `lib/WeatherCore/VaneDecoder.cpp`.
  Build that table from measured counts, not the datasheet (see the README).

## Cabling

- Anemometer and vane share one RJ11 cable. The commonly documented layout puts the anemometer on the
  middle two conductors and the vane on the outer two. Verify with a meter.
- The rain gauge has its own RJ11 cable, on the middle pair.

## Surge protection

The sensor cables are long outdoor runs, so they pick up surges and static discharge. The 1k series
resistors help. For a mast install, also add small TVS diodes from each sensor line to GND at the board,
rated for the board's logic voltage (3.3V or 5V class).

## Programming

- **Pro Mini:** use a 3.3V FTDI adapter. Either disconnect the Pro Mini from the RAK first, or make
  sure the adapter doesn't back-feed the RAK's 3.3V rail.
- **Uno, RedBoard Qwiic:** program over USB. The bootloader runs before the firmware slows the clock,
  so automatic reset and upload work as normal.
- **Pro Micro, Qwiic Pro Micro:** program over USB with the board's bootloader. The firmware turns USB
  off to save power, so the automatic reset before an upload won't reach it. Reset twice quickly to
  hold the bootloader for 8 seconds, and upload in that window. (Qwiic Pro Micro: the reset button;
  original Pro Micro: short RST to GND twice.)
  - One build serves both Pro Micro versions and the Qwiic Pro Micro: the firmware measures the
    crystal at startup.

## Breadboard setup

Build and check the circuit on a breadboard before moving it to a permanent board. The steps below
bring it up in stages, powered and monitored from the FTDI adapter, so each stage can be checked before
the next is added. The RAK is not needed until step 9.

The steps are written for the Pro Mini. On another board, use its pins from
[Pin assignments](#pin-assignments). The Uno and RedBoard are powered and monitored over USB instead
of an FTDI adapter. On a 5V board, wire the I2C link through its level shifting (see
[Supported boards](#supported-boards)) in step 9.

### Parts

- Pro Mini 3.3V/8MHz with Optiboot, headers soldered
- 3.3V FTDI adapter (its VCC pin must supply 3.3V, not 5V)
- 2× 100k, 2× 22nF, 2× 1k (reed switch inputs)
- 1× 10k (vane divider)
- 2× 4.7k (I2C pull-ups, only if nothing else on the bus has them)
- A jumper shunt, to check the alternate I2C address (step 9)
- 2× pushbuttons and a 100k potentiometer, to stand in for the sensors
- RJ11 breakout jacks, or the sensor cables cut and fitted with jumper pins

### Steps

1. **Prepare the Pro Mini.** Remove the power LED and the regulator (see
   [Board notes](#arduino-pro-mini-33v8mhz)). Solder the side headers and the FTDI header. A4 and A5
   are not on the side rows on most Pro Mini boards; solder header pins or short wires to those pads
   too.

2. **Seat the board.** Place the Pro Mini across the breadboard's centre channel. Connect its GND to
   the breadboard's ground rail and its VCC to the positive rail. This rail is the 3.3V rail for
   everything below.

3. **Power and flash.** Measure 3.3V (not 5V) between VCC and GND before going
   further. Plug in the FTDI adapter.  Then flash the debug build and open the monitor:

   ```sh
   pio run -e promini_debug -t upload
   pio device monitor -e promini_debug
   ```

   Expect `weather interface`, then one line per second with `pulses=0`. With nothing on A0 yet,
   `vane_raw` floats and means nothing.

4. **Anemometer input (D8).** Build the reed switch circuit from
   [Reed switch inputs](#reed-switch-inputs-anemometer-and-rain-identical): 100k from D8 to 3.3V, 22nF
   from D8 to GND, and 1k from D8 to one side of a pushbutton whose other side goes to GND. Press the
   button a few times within one second; `pulses` should count the presses, one per press.

5. **Rain input (D9).** Build the same circuit on D9. The debug line has no per-second rain count, so
   confirm it with a meter or scope instead: D9 sits at 3.3V and drops to near 0V while the button is
   held. The rain total itself only appears after an hour (see the README's bench checks).

6. **Vane divider (D4, A0).** Connect 10k from D4 to A0. Connect the potentiometer's wiper to A0 and one
   outer leg to GND, leaving the other leg unconnected, so it acts as a variable resistor to ground.
   Turn it through its range; `vane_raw` should move smoothly from near 0 towards ~900. To check the
   decoding, swap the pot for a fixed resistor from the table in `lib/WeatherCore/VaneDecoder.cpp`
   (33k should read about 786, i.e. north).

7. **Wait for the window to fill.** `avg`, `gust` and `dir` stay 0 for the first 10 minutes after
   power-up. After that, press the anemometer button steadily; `avg` and `gust` should rise, and `dir`
   should follow the vane setting (in 0.1°). With no presses in the last 10 minutes, direction stays
   invalid.

8. **Real sensors.** Replace the pushbuttons and pot with the sensors through the RJ11 breakouts, per
   [Cabling](#cabling): anemometer on the middle pair of its cable to the D8 circuit, vane on the outer
   pair to A0 and GND, rain gauge on the middle pair of its cable to the D9 circuit. Identify the pairs
   with a meter first. Spin the anemometer and turn the vane, and check the debug output as in steps 4
   and 6. This is also the setup for [calibrating the vane](../README.md#calibrating-the-vane).

9. **I2C to the RAK.** Flash the non-debug `promini` build (or keep the debug build if you want the
   serial output alongside). Then unplug the FTDI adapter, or leave only its TX, RX and GND connected,
   so it doesn't back-feed the RAK (see [Programming](#programming)). Connect A4 to the RAK's SDA, A5 to
   SCL, GND to GND, and the 3.3V rail to the RAK's 3.3V. Add the 4.7k pull-ups from SDA and SCL to 3.3V
   only if the RAK base has none (see [I2C bus](#i2c-bus)). On the RAK, write `0x00` to address `0x6C`
   and read 16 bytes; the first byte should be `0x57`, and `sequence` (byte 3) should go up by one each
   second. Fit a shunt across D5 and D6, reset the Pro Mini, and check that it now answers at `0x6E`
   instead.
