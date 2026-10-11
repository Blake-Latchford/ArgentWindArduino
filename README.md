# ArgentWindArduino

Firmware for a 3.3V Arduino Pro Micro weather interface board to read Argent Data wind and rain sensor assembly.
Delivers standard compliant processed results as an I2C slave.
The primary expected master device is a a MeshCore sensor.

This project has been built with the help of AI.

## Project Layout

`src/` is specific to the Pro Micro.
`lib/WeatherCore` has no hardware dependencies and builds on any target. A port to another board feeds
it one `WeatherSample` a second; see [lib/WeatherCore/docs.md](lib/WeatherCore/docs.md).

## Build, test, flash

This project uses [PlatformIO](https://platformio.org/).

```sh
pio test -e native              # unit tests on the host
pio run                         # build every environment
pio run -e promicro -t upload   # build and flash (double-tap reset first, see Programming)
```

`promicro_debug` prints each second's readings on the debug serial port (the TX/RX pins) at 250000 baud
(`pio device monitor -e promicro_debug`, through a 3.3V USB-serial adapter).

## I2C register map

To read everything, write `0x00` and read 16 bytes.
A single read of the whole map provides a consistent snapshot.
The board defaults to an I2C slave at address `0x6C` or `0x6E` with a jumper across D5 and D6.
Values are little endian.

The map is defined in [lib/WeatherCore/WeatherRegisters.h](lib/WeatherCore/WeatherRegisters.h).

# Manual Bench Validation

## Calibrating the vane

The table in `lib/WeatherCore/VaneDecoder.cpp` uses nominal datasheet resistances. To replace it with real
values:

1. Install the vane on its real cable, and flash the `promicro_debug` build.
2. Point the vane at each of the 16 headings in turn.
3. At each heading, record the `vane_raw` count from the serial output.
4. Put the recorded counts into `VANE_COUNTS`.
5. If a measured count lands outside the open/short thresholds, adjust those too.

## Bench checks

- With the debug build, spin the anemometer and confirm that `pulses` follows it.
- After the first hour (status bit 4 set), tip the rain bucket N times. Within two minutes, confirm that
  `rain_last_hour` goes up by N × 27.94 (0.01 mm).
- Check the reed-switch edges on a scope with the real cables, and adjust the RC values if needed.

# Hardware

## Supported boards

- The [SparkFun Qwiic Pro Micro USB-C](https://www.sparkfun.com/sparkfun-qwiic-pro-micro-usb-c-atmega32u4.html) (DEV-15795) is the primary supported hardware: its Qwiic port connects to the RAK without soldering the I2C wiring.
- The 3.3V/8MHz Pro Micro is also supported, by the same build.
- [Arduino Uno Q](https://docs.arduino.cc/hardware/uno-q/) support is planned for [Gusty](https://github.com/vielmetti/gusty)

The 5V/16MHz Pro Micro runs the same build, but needs an I2C level shifter from 5V to 3.3V and a
separate 5V power supply.

## Board notes

### SparkFun Qwiic Pro Micro USB-C

- ATmega32U4 with a 16MHz crystal, and an AP2112 3.3V regulator (6V maximum input on RAW).
- **Set it to 3.3V.** It ships at 5V. On the back, cut the voltage jumper's trace and bridge the center
  and right pads. The board then runs at 3.3V from its regulator, powered from USB or RAW.
  - The 16MHz crystal is beyond the 32U4's rating at 3.3V (about 10MHz). The firmware drops to 2MHz
    before `main()`, so only the bootloader and the start of startup run at 16MHz. That usually works,
    but it isn't guaranteed: check that the board starts reliably.
  - Check the brown-out fuse: a level meant for 5V would hold the chip in reset at 3.3V.
  - SparkFun's guide doesn't say whether the RAK's 3.3V can be fed straight into VCC in this mode,
    back-feeding the regulator. Check the schematic before doing that; until then, power it from USB
    or RAW.
- The Qwiic port is **not level shifted**. SDA and SCL run straight to the 32U4's D2/D3, with 2.2k
  pull-ups to the 3.3V rail (R5, R7 on the v2.0 schematic). At 3.3V that's a direct, in-spec link to
  the RAK.
  - At 5V it isn't: the 32U4's input-high threshold is then 0.7 × 5V = 3.5V, above the 3.3V bus, and
    the internal pull-ups that `Wire.begin()` turns on briefly pull towards 5V.
  - The 2.2k pull-ups are in parallel with any on the RAK side. With 4.7k on the RAK, that's about 1.5k
    combined, still within the I2C 3mA sink limit at 3.3V.
- The red power LED runs from the 3.3V rail through 1k, about 1.5mA. Remove it for field use.
- The RX and TX LEDs (PB0 and PD5) are wired from VCC to the pin, so they light when the pin is **low**.
  The firmware drives those pins high to keep them off.

### Pro Micro 3.3V/8MHz

- The voltage is marked on the back. Check it's the **3.3V/8MHz** version: it wires straight to the
  RAK's I2C bus and can run from the RAK's 3.3V rail into VCC. The 5V/16MHz version needs a level
  shifter and its own 5V supply.
- I2C is on **D2 (SDA) and D3 (SCL)**. There are no pull-ups on the board.
- Remove the red power LED for field use.
- There's no reset button. To reset, short RST to GND.

## Power

- The RAK supplies **3.3V only**. Never connect a 5V rail or a 5V signal to it.
- **Qwiic Pro Micro** (set to 3.3V): power it separately from USB or RAW (up to 6V), with a common
  ground (see its notes).
- **Pro Micro 3.3V:** feed the RAK's 3.3V into VCC, bypassing the regulator, with a common ground.
- **Qwiic cables carry 3.3V on the red wire**, from the board's own regulator. If the RAK is powered
  separately, leave the red wire unconnected (cut it, or use a cable without it), so the two 3.3V
  regulators aren't tied together. GND, SDA and SCL still connect.

## Pin assignments

`src/Board.h` is the source of truth; keep this table in step with it.

| Function | Pin | Notes |
|---|---|---|
| I2C SDA | D2 | Also on the Qwiic port. Slave address 0x6C, or 0x6E with the address jumper |
| I2C SCL | D3 | Also on the Qwiic port |
| Anemometer reed switch | D8 | Pin-change interrupt, falling edge |
| Rain gauge reed switch | D9 | Pin-change interrupt, falling edge |
| Top of vane divider | D4 | HIGH only while sampling |
| Vane divider midpoint | A0 | ADC input |
| Address jumper | D5, D6 | Shunt fitted: 0x6E. Read at reset |
| Debug serial | D1 (TX), D0 (RX) | Debug builds only (`Serial1`) |
| Power | VCC | See [Power](#power) |
| GND | GND | RAK GND, sensor grounds |

Spare: D7.

**On the header.** One long edge runs TX, RX, GND, GND, then D2–D9; the other runs RAW, GND, RST, VCC,
A3–A0, then four more. So everything except A0 is on one edge: I2C on D2–D3, D4, then the D5–D6 jumper,
D7 spare, then the reed switches on D8–D9 at the end.

**Why these pins:**
- The reed switches need pin-change interrupts on one port. The 32U4 has them only on PORTB, and D8/D9
  are on PORTB, so both switches share one interrupt.
- The vane drive can be any digital output and the vane input any analog input.
- The jumper needs two adjacent free pins: no free pin sits next to a GND, so D6 is driven low to act
  as the jumper's ground while the firmware reads it at reset.
- `Serial` is USB, which the firmware turns off; debug output goes to `Serial1` on the TX/RX pins and
  needs a 3.3V USB-serial adapter.

## I2C bus

- Exactly one set of pull-ups on the 3.3V side of the bus, sized for it. The Qwiic Pro Micro's 2.2k
  resistors count as pull-ups. On the Pro Micro 3.3V, if the RAK base already has them, add none;
  otherwise 4.7k from SDA and from SCL to 3.3V.
- The firmware turns off the internal pull-ups that `Wire.begin()` enables. To keep them on instead (a
  short bus with no external pull-ups), build with `-DWEATHER_I2C_EXTERNAL_PULLUPS=0` in
  `platformio.ini`. Never do that on a 5V Pro Micro wired straight to the RAK: the internal pull-ups go
  to 5V.
- The slave address is 0x6C. A 2-pin jumper shunt across D5 and D6 changes it to 0x6E, for a second
  unit or a clash with another device on the bus. The firmware reads the jumper only at reset. To use
  different addresses, add `-DWEATHER_I2C_PRIMARY_ADDRESS=0x..` (jumper off) or
  `-DWEATHER_I2C_SECONDARY_ADDRESS=0x..` (jumper on) to `build_flags` in `platformio.ini`.
- Run the bus at 100kHz, not 400kHz. The firmware runs the CPU at 2MHz, and the TWI slave needs a CPU
  clock of at least 16x SCL.

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
  Build that table from measured counts, not the datasheet (see
  [Calibrating the vane](#calibrating-the-vane)).

## Cabling

- Anemometer and vane share one RJ11 cable. The commonly documented layout puts the anemometer on the
  middle two conductors and the vane on the outer two. Verify with a meter.
- The rain gauge has its own RJ11 cable, on the middle pair.

## Surge protection

The sensor cables are long outdoor runs, so they pick up surges and static discharge. The 1k series
resistors help. For a mast install, also add small TVS diodes from each sensor line to GND at the board,
rated for the board's logic voltage (3.3V or 5V class).

## Programming

- Program over USB with the board's bootloader. The firmware turns USB off to save power, so the
  automatic reset before an upload won't reach it. Reset twice quickly to hold the bootloader for 8
  seconds, and upload in that window (Qwiic Pro Micro: the reset button; Pro Micro: short RST to GND
  twice).
- One build serves both boards: the firmware measures the crystal at startup.
- If the RAK powers the board, unplug USB while it's connected, so USB doesn't back-feed the RAK's 3.3V
  rail.

## Breadboard setup

Build and check the circuit on a breadboard before moving it to a permanent board. The steps below
bring it up in stages, powered over USB and monitored through a USB-serial adapter, so each stage can
be checked before the next is added. The RAK is not needed until step 9.

The steps are written for the Qwiic Pro Micro. On the Pro Micro 3.3V, solder its headers, reset it by
shorting RST to GND, and wire I2C to D2/D3 with pull-ups in step 9.

### Parts

- SparkFun Qwiic Pro Micro USB-C, set to 3.3V, headers soldered
- USB-C cable
- 3.3V USB-serial adapter, for the debug output (only its TX, RX and GND are used)
- 2× 100k, 2× 22nF, 2× 1k (reed switch inputs)
- 1× 10k (vane divider)
- A Qwiic cable, with its red wire cut or a cable to pins at the RAK end
- A jumper shunt, to check the alternate I2C address (step 9)
- 2× pushbuttons and a 100k potentiometer, to stand in for the sensors
- RJ11 breakout jacks, or the sensor cables cut and fitted with jumper pins

### Steps

1. **Prepare the board.** Set the voltage jumper to 3.3V and remove the power LED (see
   [Board notes](#sparkfun-qwiic-pro-micro-usb-c)). Solder the side headers.

2. **Seat the board.** Place the board across the breadboard's centre channel. Connect its GND to the
   breadboard's ground rail and its VCC to the positive rail. This rail is the 3.3V rail for everything
   below.

3. **Power and flash.** Plug in USB-C, and measure 3.3V (not 5V) between VCC and GND before going
   further. Connect the USB-serial adapter's GND to GND, its RX to the board's TX and its TX to the
   board's RX. Double-tap reset, then flash the debug build and open the monitor on the adapter's port:

   ```sh
   pio run -e promicro_debug -t upload
   pio device monitor -e promicro_debug
   ```

   Expect `weather interface`, then one line per second with `pulses=0`. With nothing on A0 yet,
   `vane_raw` floats and means nothing.

4. **Anemometer input (D8).** Build the reed switch circuit from
   [Reed switch inputs](#reed-switch-inputs-anemometer-and-rain-identical): 100k from D8 to 3.3V, 22nF
   from D8 to GND, and 1k from D8 to one side of a pushbutton whose other side goes to GND. Press the
   button a few times within one second; `pulses` should count the presses, one per press.

5. **Rain input (D9).** Build the same circuit on D9. The debug line has no per-second rain count, so
   confirm it with a meter or scope instead: D9 sits at 3.3V and drops to near 0V while the button is
   held. The rain total itself only appears after an hour (see [Bench checks](#bench-checks)).

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
   and 6. This is also the setup for [calibrating the vane](#calibrating-the-vane).

9. **I2C to the RAK.** Flash the non-debug `promicro` build (or keep the debug build if you want the
   serial output alongside). Connect the Qwiic port to the RAK's SDA, SCL and GND, leaving the red wire
   unconnected while the board runs from USB (see [Power](#power)). The board's 2.2k pull-ups serve the
   bus (see [I2C bus](#i2c-bus)). On the RAK, write `0x00` to address `0x6C` and read 16 bytes; the
   first byte should be `0x57`, and `sequence` (byte 3) should go up by one each second. Fit a shunt
   across D5 and D6, reset the board, and check that it now answers at `0x6E` instead.




- **Clock and sleep:** before `main()`, the chip times its crystal against the watchdog oscillator and
  divides it to 2MHz (8MHz by 4, 16MHz by 8), so `F_CPU` is 2MHz throughout. Between interrupts it sits
  in idle sleep. Timer0 is off, so `millis()` and `delay()` don't work. SPI, Timers 3 and 4, USB, the
  comparator and (outside debug builds) the USART are off too. The ADC is powered only while the vane is
  being sampled.
- **Time base:** Timer1 in CTC mode interrupts at 1Hz.
- **Hang recovery:** the watchdog runs in interrupt-then-reset mode with a 2s timeout. The main loop
  re-arms it on each tick. If the watchdog does reset the chip, the next boot sets a status bit.