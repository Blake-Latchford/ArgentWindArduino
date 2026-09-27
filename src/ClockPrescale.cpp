#include <avr/io.h>
#include <avr/power.h>
#include <avr/wdt.h>

#if F_CPU != 2000000L
#error "Build with F_CPU=2000000L: the 8MHz clock is prescaled to 2MHz before main()"
#endif

// Runs before the C runtime initializes memory: take the watchdog off (a watchdog reset leaves it
// running), then drop the 8MHz clock to 2MHz. 2MHz is the floor: the TWI slave needs a CPU clock of at
// least 16x SCL to keep up with a 100kHz bus.
static void earlyInit() __attribute__((naked, used, section(".init3")));
static void earlyInit() {
  MCUSR = 0;
  wdt_disable();
  clock_prescale_set(clock_div_4);
}
