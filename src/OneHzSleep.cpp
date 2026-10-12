#include "OneHzSleep.h"

#include <avr/interrupt.h>
#include <avr/io.h>
#include <avr/sleep.h>

static volatile uint8_t pending_ticks;

// CTC mode, clock / 64, counting 0 to COUNTS_PER_SECOND - 1.
void OneHzSleep::begin() {
  set_sleep_mode(SLEEP_MODE_IDLE);
  TCCR1A = 0;
  TCCR1B = _BV(WGM12) | _BV(CS11) | _BV(CS10);
  OCR1A = COUNTS_PER_SECOND - 1;
  TCNT1 = 0;
  TIFR1 = _BV(OCF1A);
  TIMSK1 = _BV(OCIE1A);
}

void OneHzSleep::sleepOneSecond() {
  for (;;) {
    // Check with interrupts off so a tick can't slip in between the check and the sleep; sleep_cpu()
    // right after sei() runs before any pending interrupt.
    cli();
    if (pending_ticks > 0) {
      pending_ticks = 0;
      sei();
      return;
    }
    sleep_enable();
    sei();
    sleep_cpu();
    sleep_disable();
  }
}

ISR(TIMER1_COMPA_vect) {
  if (pending_ticks < 0xFF) {
    pending_ticks++;
  }
}
