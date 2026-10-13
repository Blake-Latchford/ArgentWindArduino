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

To read everything, read 16 bytes. The register pointer starts at `0x00` and returns there after every
read, so writing `0x00` first is optional. To start part way through the map, write the register offset,
then read.
A single read of the whole map provides a consistent snapshot.
The board [defaults](AgilentWindArduino/src/IICServer.cpp) to an I2C slave at address `0x6C` or `0x6E` with a jumper across D5 and D6.
Values are little endian.

The register map is defined in [lib/WeatherCore/WeatherRegisters.h](lib/WeatherCore/WeatherRegisters.h).
This firmware only supports a 100kHz I2C bus, which is the maximum the 2MHz clock can support.

# Manual Bench Validation

## Calibrating the vane

The table in `lib/WeatherCore/src/VaneDecoder.cpp` uses nominal datasheet resistances. To replace it with real
values:

1. Install the vane on its real cable, and flash the `promicro_debug` build.
2. Point the vane at each of the 16 headings in turn.
3. At each heading, record the `vane_raw` count from the serial output.
4. Put the recorded counts into `VANE_COUNTS`.
5. If a measured count lands outside the open/short thresholds, adjust those too.

## Bench checks

- With the debug build, spin the anemometer and confirm that `pulses` follows it.
- After the first hour (status bit 2 set), tip the rain bucket N times. Within two minutes, confirm that
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

## Assembly

### Parts

- SparkFun Qwiic Pro Micro USB-C
- 1× 10k (vane divider)
- 2× RJ11 jacks, for the sensor cables
- A Qwiic cable to the RAK

### Steps
Pin assignments live in [src/Board.h](src/Board.h)

TODO: This should really be illustrated.

1. **Set the board to 3.3V.** Cut the trace on the bottom between VCC and 5V, then bridge VCC to 3.3V.
   Failure to do this could cause the Pro Micro to drive the I2C bus at 5V, possibly damaging the
   master. Remove the red power LED, which draws about 1.5mA.
2. **Find the sensor pairs.** Plug each sensor cable into its RJ11 jack, and identify the conductors
   with a meter on the jack's pins before soldering anything (see [Cabling](#cabling)). None of the
   sensors has a polarity, so either wire of a pair can go to the pin.
   - **Anemometer** (the shared cable, usually the middle two conductors): turn the cups slowly. The pair
     switches between open and near 0Ω, closing once per turn.
   - **Vane** (the shared cable, usually the outer two): the pair reads a resistance between about 690Ω
     and 120k, which changes as you turn the vane.
   - **Rain gauge** (its own cable, usually the middle two): tip the bucket by hand. The pair closes
     briefly at each tip.
3. **Wire the vane divider.** Solder the 10k between D4 and A0 (see [Wind vane](#wind-vane)). They're
   on opposite edges of the board: D4 is with D2-D9 on the TX/RX edge, and A0 is with A0-A3 on the
   RAW/VCC edge. Run the resistor across the underside, or use a short insulated wire for one leg.
4. **Wire the shared jack.** Anemometer pair: one wire to D8, the other to GND. Vane pair: one wire to
   A0, the other to GND. The GND pins next to TX/RX are on the same edge as D8.
5. **Wire the rain gauge jack.** One wire of its pair to D9, the other to GND
6. **Check the wiring.** Before powering up, unplug the sensor cables and check with a meter:
   - Continuity from each jack pin you used to its board pin: D8, D9, A0 and GND.
   - No short between neighbouring pins (in particular D8 and D9), or from D8, D9 or A0 to GND.
   - 10k between D4 and A0.
7. **Flash the build.** Double-tap reset, then `pio run -e promicro -t upload`.

On a standard 3.3V Pro Micro, wire I2C to D2 (SDA) and D3 (SCL) in place of the Qwiic cable, and reset
it by shorting RST to GND.

## Wind vane

```
D4 ──[10k]──┬── A0
            │
          vane
            │
           GND
```

- The divider draws current only while sampling.
- The ADC reference is VCC, the same supply that drives the divider, so the reading is ratiometric: the
  same vane position gives the same count on a 3.3V or a 5V board, and supply drift cancels.
- The firmware maps readings to 16 headings using the table in `lib/WeatherCore/src/VaneDecoder.cpp`.