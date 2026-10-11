#include "ReedSwitches.h"

#include <avr/interrupt.h>
#include <avr/io.h>

#include "Board.h"

// Both switches are on PORTB (see Board.h). PORTB is pin-change group 0 on both chips, and its PCINT
// bits match its port bits, so a pin's PCMSK bit is also its PINB bit.
static_assert(digitalPinToPCICRbit(BOARD_ANEMOMETER_PIN) == 0, "anemometer pin must be on PORTB");
static_assert(digitalPinToPCICRbit(BOARD_RAIN_PIN) == 0, "rain pin must be on PORTB");
#define ANEMOMETER_BIT _BV(digitalPinToPCMSKbit(BOARD_ANEMOMETER_PIN))
#define RAIN_BIT _BV(digitalPinToPCMSKbit(BOARD_RAIN_PIN))

// Free-running totals: the ISR only increments them and take() reports the change since its last
// call, so they may wrap. A single-byte read is atomic on AVR, so take() needs no cli()/sei().
static volatile uint8_t wind_total;
static volatile uint8_t rain_total;
static uint8_t last_port_bitmask;

void ReedSwitches::begin() {
  // Inputs with the internal pull-ups off; the external 100k ones draw less while a switch sits closed.
  pinMode(BOARD_ANEMOMETER_PIN, INPUT);
  pinMode(BOARD_RAIN_PIN, INPUT);

  // Seed the ISR's edge detection with the pins' current levels. Left at 0, the first falling edge
  // would look like no change (0 -> 0) and be missed.
  last_port_bitmask = PINB;

  // Pin-change interrupts fire on any edge, rising or falling; the ISR picks out the falling ones.
  // PCMSK0 selects which PORTB pins raise PCINT0_vect.
  PCMSK0 = ANEMOMETER_BIT | RAIN_BIT;
  // Clear any change latched while the pins were being set up (writing 1 clears the flag), so the
  // ISR doesn't run once as soon as it's enabled.
  PCIFR = _BV(PCIF0);
  // Enable the PORTB group. Any of these edges also wakes the CPU from OneHzSleep's idle sleep. Leave
  // the other groups disabled.
  PCICR |= _BV(PCIE0);
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
ISR(PCINT0_vect) {
  uint8_t port_bitmask = PINB;
  uint8_t fell = last_port_bitmask & ~port_bitmask;
  last_port_bitmask = port_bitmask;

  if (fell & ANEMOMETER_BIT) {
    wind_total++;
  }
  if (fell & RAIN_BIT) {
    rain_total++;
  }
}
