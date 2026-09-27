#include "Watchdog.h"

#include <avr/interrupt.h>
#include <avr/wdt.h>
#include <util/atomic.h>

// Set by the watchdog interrupt just before the watchdog resets the chip; survives the reset because
// .noinit isn't cleared at startup.
#define WATCHDOG_MAGIC 0xA5
static uint8_t watchdog_marker __attribute__((section(".noinit")));

bool Watchdog::causedLastReset() {
  bool fired = watchdog_marker == WATCHDOG_MAGIC;
  watchdog_marker = 0;
  return fired;
}

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

ISR(WDT_vect) { watchdog_marker = WATCHDOG_MAGIC; }
