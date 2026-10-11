#include "OneHzSleep.h"

#include <avr/interrupt.h>
#include <avr/io.h>
#include <avr/sleep.h>

static volatile uint8_t pending_ticks;

// CTC mode: F_CPU / 64 = 31250 counts per second at 2MHz.
void OneHzSleep::begin() {
  set_sleep_mode(SLEEP_MODE_IDLE);
  TCCR1A = 0;
  TCCR1B = _BV(WGM12) | _BV(CS11) | _BV(CS10);
  OCR1A = F_CPU / 64 - 1;
  TCNT1 = 0;
  TIFR1 = _BV(OCF1A);
  TIMSK1 = _BV(OCIE1A);
}

uint8_t OneHzSleep::sleepOneSecond() {
  for (;;) {
    // Check with interrupts off so a tick can't slip in between the check and the sleep; sleep_cpu()
    // right after sei() runs before any pending interrupt.
    cli();
    uint8_t ticks = pending_ticks;
    if (ticks > 0) {
      pending_ticks = 0;
      sei();
      return ticks;
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
