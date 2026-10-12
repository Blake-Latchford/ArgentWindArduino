#include <avr/io.h>
#include <avr/power.h>
#include <avr/wdt.h>

#if F_CPU != 2000000L
#error "Build with F_CPU=2000000L: the crystal clock is prescaled to 2MHz before main()"
#endif

static bool crystalIs16MHz();

// Before main(), so all code runs at 2MHz: the lowest clock at which the TWI slave keeps up with a
// 100kHz bus. A watchdog reset leaves the watchdog running, so it's stopped first. The crystal is
// measured rather than configured so one build serves 8MHz and 16MHz boards.
static void earlyInit() __attribute__((naked, used, section(".init3")));
static void earlyInit() {
  MCUSR = 0;
  wdt_disable();
  clock_prescale_set(crystalIs16MHz() ? clock_div_8 : clock_div_4);
}

// The watchdog's oscillator is independent of the crystal, so one of its periods measures the crystal.
// 8MHz and 16MHz are a factor of two apart, which tolerates the watchdog being ~40% off.
//
// Interrupts are still off, so the watchdog flag is polled. .data and .bss aren't initialized yet, so
// no globals.
static bool crystalIs16MHz() {
  TCCR1A = 0;
  TCCR1B = _BV(CS11);

  // Interrupt mode, so the watchdog doesn't reset the chip.
  WDTCSR = _BV(WDCE) | _BV(WDE);
  WDTCSR = _BV(WDIF) | _BV(WDIE);

  // Sync to a watchdog edge first, or the timed period would be a partial one.
  while (!(WDTCSR & _BV(WDIF))) {
  }
  TCNT1 = 0;
  WDTCSR = _BV(WDIF) | _BV(WDIE);
  while (!(WDTCSR & _BV(WDIF))) {
  }
  uint16_t counts = TCNT1;

  // A pending flag would fire as soon as Watchdog::begin() enables the interrupt, and spend the
  // interrupt stage before the first timeout.
  WDTCSR = _BV(WDIF);
  TCCR1B = 0;
  TCNT1 = 0;

  return counts > 22627;  // between 16000 (8MHz) and 32000 (16MHz), geometrically
}
