#include "ReedSwitches.h"

#include <avr/interrupt.h>
#include <avr/io.h>

// Both switches are on PORTD, so one pin-change vector covers them.
#define ANEMOMETER_BIT PD2
#define RAIN_BIT PD3

// Free-running totals: the ISR only increments them and take() reports the change since its last
// call, so they may wrap. A single-byte read is atomic on AVR, so take() needs no cli()/sei().
static volatile uint8_t wind_total;
static volatile uint8_t rain_total;
static uint8_t last_port_d;

void ReedSwitches::begin() {
  // Inputs with the internal pull-ups off; the external 100k ones draw less while a switch sits closed.
  // A clear DDRD bit makes the pin an input; on an input, a clear PORTD bit disables its pull-up.
  DDRD &= ~(_BV(ANEMOMETER_BIT) | _BV(RAIN_BIT));
  PORTD &= ~(_BV(ANEMOMETER_BIT) | _BV(RAIN_BIT));

  // Seed the ISR's edge detection with the pins' current levels. Left at 0, the first falling edge
  // would look like no change (0 -> 0) and be missed.
  last_port_d = PIND;

  // Pin-change interrupts fire on any edge, rising or falling; the ISR picks out the falling ones.
  // PCMSK2 selects which PORTD pins raise PCINT2_vect: PCINT18 is PD2 (anemometer), PCINT19 is PD3
  // (rain).
  PCMSK2 = _BV(PCINT18) | _BV(PCINT19);
  // Clear any change latched while the pins were being set up (writing 1 clears the flag), so the
  // ISR doesn't run once as soon as it's enabled.
  PCIFR = _BV(PCIF2);
  // Enable the PCINT2 group (PCINT16-23, i.e. PORTD). Any of these edges also wakes the CPU from
  // OneHzSleep's idle sleep. Leave the other groups disabled.
  PCICR |= _BV(PCIE2);
}

ReedSwitches::Counts ReedSwitches::take() {
  static uint8_t last_wind_total;
  static uint8_t last_rain_total;

  uint8_t wind = wind_total;
  uint8_t rain = rain_total;
  // Unsigned subtraction handles a wrap, e.g. 3 - 250 = 9.
  Counts counts = {uint8_t(wind - last_wind_total), uint8_t(rain - last_rain_total)};
  last_wind_total = wind;
  last_rain_total = rain;
  return counts;
}

// A switch closing pulls its pin to GND, so a pulse is a falling edge.
ISR(PCINT2_vect) {
  uint8_t port_d = PIND;
  uint8_t fell = last_port_d & ~port_d;
  last_port_d = port_d;

  if (fell & _BV(ANEMOMETER_BIT)) {
    wind_total++;
  }
  if (fell & _BV(RAIN_BIT)) {
    rain_total++;
  }
}
