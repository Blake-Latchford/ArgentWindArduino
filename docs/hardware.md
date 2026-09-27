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
