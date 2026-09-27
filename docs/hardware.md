# Hardware

**Board:** Arduino Pro Mini, **3.3V / 8MHz** (ATmega328P), with Optiboot.

- Remove the power LED.
- Remove the onboard regulator, or bypass it by feeding VCC directly.
- Power from the RAK's **3.3V rail**, with a common ground.

## Pin assignments

| Pro Mini pin | Connects to | Notes |
|---|---|---|
| VCC | RAK 3.3V | Feed VCC directly, not RAW |
| GND | RAK GND, sensor grounds | |
| A4 (SDA) | RAK I2C SDA | Slave address 0x42 |
| A5 (SCL) | RAK I2C SCL | |
| D2 | Anemometer reed switch | Pin-change interrupt, falling edge |
| D3 | Rain gauge reed switch | Pin-change interrupt, falling edge |
| D4 | Top of vane divider | HIGH only while sampling |
| A0 | Vane divider midpoint | ADC input |

## I2C bus

- Exactly one set of pull-ups on the bus. If the RAK base already has them, add none; otherwise 4.7k
  from SDA and from SCL to 3.3V.
- The firmware turns off the internal pull-ups that `Wire.begin()` enables. To keep them on instead (a
  short bus with no external pull-ups), build with `-DWEATHER_I2C_EXTERNAL_PULLUPS=0` in
  `platformio.ini`.
- Run the bus at 100kHz, not 400kHz. The Pro Mini runs at 2MHz, and the TWI slave needs a CPU clock of
  at least 16x SCL.

## Reed switch inputs (anemometer and rain, identical)

```
3.3V ──[100k]──┬──────────── D2 / D3
               │
             [22nF]
               │
GND ───────────┴── switch ──[1k]── (pin node)
```

- 100k pull-up from the pin to 3.3V: 33µA while the switch sits closed, against ~100µA with the
  internal pull-up.
- 22nF from the pin to GND debounces. With the 100k that's about a 2ms rise, fast enough for gust-level
  pulse rates of ~60Hz.
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
- The ADC reference is VCC, so the reading is ratiometric and supply drift cancels.
- The Pro Mini maps readings to 16 headings using the table in `lib/WeatherCore/VaneDecoder.cpp`. Build
  that table from measured counts, not the datasheet (see the README).

## Cabling

- Anemometer and vane share one RJ11 cable. The commonly documented layout puts the anemometer on the
  middle two conductors and the vane on the outer two. Verify with a meter.
- The rain gauge has its own RJ11 cable, on the middle pair.

## Surge protection

The sensor cables are long outdoor runs, so they pick up surges and static discharge. The 1k series
resistors help. For a mast install, also add small TVS diodes (3.3V class) from each sensor line to GND
at the board.

## Programming

Use a 3.3V FTDI adapter. Either disconnect the Pro Mini from the RAK first, or make sure the adapter
doesn't back-feed the RAK's 3.3V rail.

## Breadboard setup

Build and check the circuit on a breadboard before moving it to a permanent board. The steps below
bring it up in stages, powered and monitored from the FTDI adapter, so each stage can be checked before
the next is added. The RAK is not needed until step 9.

### Parts

- Pro Mini 3.3V/8MHz with Optiboot, headers soldered
- 3.3V FTDI adapter (its VCC pin must supply 3.3V, not 5V)
- 2× 100k, 2× 22nF, 2× 1k (reed switch inputs)
- 1× 10k (vane divider)
- 2× 4.7k (I2C pull-ups, only if nothing else on the bus has them)
- 2× pushbuttons and a 100k potentiometer, to stand in for the sensors
- RJ11 breakout jacks, or the sensor cables cut and fitted with jumper pins

### Steps

1. **Prepare the Pro Mini.** Remove the power LED and the regulator (see the top of this page). Solder
   the side headers and the FTDI header. A4 and A5 are not on the side rows on most Pro Mini boards;
   solder header pins or short wires to those pads too.

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

4. **Anemometer input (D2).** Build the reed switch circuit from
   [Reed switch inputs](#reed-switch-inputs-anemometer-and-rain-identical): 100k from D2 to 3.3V, 22nF
   from D2 to GND, and 1k from D2 to one side of a pushbutton whose other side goes to GND. Press the
   button a few times within one second; `pulses` should count the presses, one per press.

5. **Rain input (D3).** Build the same circuit on D3. The debug line has no per-second rain count, so
   confirm it with a meter or scope instead: D3 sits at 3.3V and drops to near 0V while the button is
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
   [Cabling](#cabling): anemometer on the middle pair of its cable to the D2 circuit, vane on the outer
   pair to A0 and GND, rain gauge on the middle pair of its cable to the D3 circuit. Identify the pairs
   with a meter first. Spin the anemometer and turn the vane, and check the debug output as in steps 4
   and 6. This is also the setup for [calibrating the vane](../README.md#calibrating-the-vane).

9. **I2C to the RAK.** Flash the non-debug `promini` build (or keep the debug build if you want the
   serial output alongside). Then unplug the FTDI adapter, or leave only its TX, RX and GND connected,
   so it doesn't back-feed the RAK (see [Programming](#programming)). Connect A4 to the RAK's SDA, A5 to
   SCL, GND to GND, and the 3.3V rail to the RAK's 3.3V. Add the 4.7k pull-ups from SDA and SCL to 3.3V
   only if the RAK base has none (see [I2C bus](#i2c-bus)). On the RAK, write `0x00` to address `0x42`
   and read 16 bytes; the first byte should be `0x57`, and `sequence` (byte 3) should go up by one each
   second.
