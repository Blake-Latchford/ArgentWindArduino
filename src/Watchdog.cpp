#include "Watchdog.h"

#include <avr/interrupt.h>
#include <avr/wdt.h>
#include <util/atomic.h>

void Watchdog::begin() {
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
    wdt_reset();
    WDTCSR = _BV(WDCE) | _BV(WDE);
    WDTCSR = _BV(WDIE) | _BV(WDE) | _BV(WDP2) | _BV(WDP1) | _BV(WDP0);
  }
}

// The hardware clears WDIE when the interrupt fires; setting it again restores the interrupt stage.
void Watchdog::rearm() {
  wdt_reset();
  WDTCSR |= _BV(WDIE);
}

EMPTY_INTERRUPT(WDT_vect);
