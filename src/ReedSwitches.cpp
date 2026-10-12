#include "ReedSwitches.h"

#include <avr/interrupt.h>
#include <avr/io.h>

#include "Board.h"
#include "OneHzSleep.h"
#include "PulseCounter.h"

// Both switches are on PORTB (see Board.h). PORTB is pin-change group 0, and its PCINT bits match its
// port bits, so a pin's PCMSK bit is also its PINB bit.
static_assert(digitalPinToPCICRbit(BOARD_ANEMOMETER_PIN) == 0, "anemometer pin must be on PORTB");
static_assert(digitalPinToPCICRbit(BOARD_RAIN_PIN) == 0, "rain pin must be on PORTB");
#define ANEMOMETER_BIT _BV(digitalPinToPCMSKbit(BOARD_ANEMOMETER_PIN))
#define RAIN_BIT _BV(digitalPinToPCMSKbit(BOARD_RAIN_PIN))

// Debounce: a closure is counted on its first falling edge, then its pin is ignored until a lockout
// ends, which covers the switch's bounce. The anemometer's is well under its fastest pulse spacing (16ms
// at 150 km/h); rain tips are seconds apart, so its lockout can also cover a tip's magnet passing the
// switch. Each lockout runs on its own Timer1 compare channel, B for wind and C for rain.
#define ANEMOMETER_LOCKOUT_MS 5
#define RAIN_LOCKOUT_MS 100

static constexpr uint16_t lockoutCounts(uint16_t ms) {
  return (uint32_t)OneHzSleep::COUNTS_PER_SECOND * ms / 1000;
}
static_assert(lockoutCounts(RAIN_LOCKOUT_MS) < OneHzSleep::COUNTS_PER_SECOND, "a lockout must be under 1s");

static PulseCounter wind;
static PulseCounter rain;
static uint8_t last_port_bitmask;

// The Timer1 count `counts` from now, wrapping at the end of its 1s cycle.
static uint16_t timer1After(uint16_t counts) {
  uint16_t target = TCNT1 + counts;
  if (target >= OneHzSleep::COUNTS_PER_SECOND) {
    target -= OneHzSleep::COUNTS_PER_SECOND;
  }
  return target;
}

void ReedSwitches::begin() {
  pinMode(BOARD_ANEMOMETER_PIN, INPUT_PULLUP);
  pinMode(BOARD_RAIN_PIN, INPUT_PULLUP);

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

ReedSwitches::Counts ReedSwitches::take() { return {wind.take(), rain.take()}; }

// A switch closing pulls its pin to GND, so a pulse is a falling edge. A pin in its lockout is out of
// PCMSK0, so its edges are ignored.
ISR(PCINT0_vect) {
  uint8_t port_bitmask = PINB;
  uint8_t fell = last_port_bitmask & ~port_bitmask & PCMSK0;
  last_port_bitmask = port_bitmask;

  // Start the lockout: stop listening to the pin, and set its compare to fire when the lockout ends.
  // The compare flag is set on every pass of the count, so clear it before enabling the interrupt.
  if (fell & ANEMOMETER_BIT) {
    wind.increment();
    PCMSK0 &= ~ANEMOMETER_BIT;
    OCR1B = timer1After(lockoutCounts(ANEMOMETER_LOCKOUT_MS));
    TIFR1 = _BV(OCF1B);
    TIMSK1 |= _BV(OCIE1B);
  }
  if (fell & RAIN_BIT) {
    rain.increment();
    PCMSK0 &= ~RAIN_BIT;
    OCR1C = timer1After(lockoutCounts(RAIN_LOCKOUT_MS));
    TIFR1 = _BV(OCF1C);
    TIMSK1 |= _BV(OCIE1C);
  }
}

// End of a lockout: take the pin's level now as the reference for its next edge, so a switch that's
// still closed isn't counted again, and listen to the pin again.
static void endLockout(uint8_t bit) {
  last_port_bitmask = (last_port_bitmask & ~bit) | (PINB & bit);
  PCMSK0 |= bit;
}

ISR(TIMER1_COMPB_vect) {
  TIMSK1 &= ~_BV(OCIE1B);
  endLockout(ANEMOMETER_BIT);
}

ISR(TIMER1_COMPC_vect) {
  TIMSK1 &= ~_BV(OCIE1C);
  endLockout(RAIN_BIT);
}
