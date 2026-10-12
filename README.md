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
The board [defaults](AgilentWindArduino/src/IICServer.cpp) to an I2C slave at address `0x6C` or `0x6E` with a jumper across D5 and D6.
Values are little endian.

The register map is defined in [lib/WeatherCore/WeatherRegisters.h](lib/WeatherCore/WeatherRegisters.h).
This firmware only supports a 100kHz I2C bus, which is the maximum the 2MHz clock can support.

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
- Check the reed-switch edges on a scope with the real cables. Bounce should end well inside the
  debounce lockouts (see [Reed switch inputs](#reed-switch-inputs-anemometer-and-rain-identical)).

# Hardware

## Supported boards

The [SparkFun Qwiic Pro Micro USB-C](https://www.sparkfun.com/sparkfun-qwiic-pro-micro-usb-c-atmega32u4.html) (DEV-15795) is the primary supported hardware.
The goal is to allow a single QWIIC to provide power and data, though that needs to be proven.
A standard 3.3V Pro Micro should also work with a bit of extra soldering.

I might switch to a pro mini at some point.

## Hardware mofications

Do **not** connect the 5V from a Rak WizBlock to the sensor board.

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
- 2× 10nF (reed switch inputs)
- 1× 10k (vane divider)
- A Qwiic cable, with its red wire cut or a cable to pins at the RAK end
- A jumper shunt, to check the alternate I2C address (step 9)
- 2× pushbuttons and a 100k potentiometer, to stand in for the sensors
- RJ11 breakout jacks, or the sensor cables cut and fitted with jumper pins

### Steps

1. **Prepare the board.** Set the voltage jumper to 3.3V and remove the power LED (see
   [Board notes](#sparkfun-qwiic-pro-micro-usb-c)). Solder the side headers.

The SparkFun Qwiic Pro Micro USB-C must have its supply voltage changed to 3.3V.
This can be done by cutting the trace on the bottom between VCC and 5V, then
bridging VCC to 3.3V. Failure to do this could cause the pro micro
to drive the I2C bus at 5V, possibly damaging the master.


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
   [Reed switch inputs](#reed-switch-inputs-anemometer-and-rain-identical): 10nF from D8 to GND, and a
   pushbutton from D8 to GND. Press the
   button a few times within one second; `pulses` should count the presses, one per press. A
   pushbutton can bounce for longer than a reed switch, so an occasional double count here is the
   button, not the circuit.

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


## Pin assignments

Pin assignments live in [src/Board.h](src/Board.h)


## Reed switch inputs (anemometer and rain, identical)

```
reed pin (see Pin assignments) ──┬────────┐
                                 │        │
                              [10nF]    switch
                                 │        │
GND ─────────────────────────────┴────────┘
```

- The pull-up is the pin's internal one (20-50k), so there's no pull-up resistor. It draws about 100µA
  at 3.3V while a switch sits closed, as the anemometer can when it stops.
- Debouncing is in software (`src/ReedSwitches.cpp`): the firmware counts a closure on its first
  falling edge, then ignores that pin for a lockout, 5ms for the anemometer and 100ms for the rain gauge.
  The anemometer's fastest pulses are about 16ms apart (150 km/h), and rain tips are seconds apart.
- 10nF from the pin to GND filters noise from the long outdoor cables, so a spike of a few µs isn't
  counted as a pulse. With the internal pull-up that's about a 0.2-0.5ms rise, short against the
  lockouts. The cable's own capacitance, roughly 50pF per metre, adds to it.
- There's no series resistor, so a closing switch discharges the capacitor straight through its
  contacts. At 10nF and 3.3V that's a tiny amount of energy. It also leaves nothing limiting the current
  from a surge on the cable into the pin (see [Surge protection](#surge-protection)).
- The other side of each switch goes to GND; pulses are falling edges.

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

The sensor cables are long outdoor runs, so they pick up surges and static discharge. The reed inputs'
10nF capacitors absorb fast spikes, but nothing limits the current into the pins. For a mast install, also add small TVS diodes from each sensor line to GND at the board,
rated for the board's logic voltage (3.3V or 5V class).

## Programming

- Program over USB with the board's bootloader. The firmware turns USB off to save power, so the
  automatic reset before an upload won't reach it. Reset twice quickly to hold the bootloader for 8
  seconds, and upload in that window (Qwiic Pro Micro: the reset button; Pro Micro: short RST to GND
  twice).
- One build serves both boards: the firmware measures the crystal at startup.
- If the RAK powers the board, unplug USB while it's connected, so USB doesn't back-feed the RAK's 3.3V
  rail.



- **Clock and sleep:** before `main()`, the chip times its crystal against the watchdog oscillator and
  divides it to 2MHz (8MHz by 4, 16MHz by 8), so `F_CPU` is 2MHz throughout. Between interrupts it sits
  in idle sleep. Timer0 is off, so `millis()` and `delay()` don't work. SPI, Timers 3 and 4, USB, the
  comparator and (outside debug builds) the USART are off too. The ADC is powered only while the vane is
  being sampled.
- **Time base:** Timer1 in CTC mode interrupts at 1Hz on compare A. Its compares B and C time the reed
  switch debounce lockouts.
- **Hang recovery:** the watchdog runs in interrupt-then-reset mode with a 2s timeout. The main loop
  re-arms it on each tick. If the watchdog does reset the chip, the next boot sets a status bit.