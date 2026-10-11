#pragma once

#include <stdint.h>

// Counts reed switch closures in an interrupt handler and hands the main loop the count since its last
// take(). The total is a single byte that only the interrupt handler writes, so take() needs no
// interrupt lock on AVR or Cortex-M: a byte read is atomic on both.
class PulseCounter {
public:
  // From the interrupt handler, once per debounced closure (a falling edge on the Argent Data switches).
  void increment() { _total = _total + 1; }

  // From the main loop: closures since the last call. Correct up to 255 per call; beyond that it wraps.
  uint8_t take() {
    uint8_t total = _total;
    // Unsigned subtraction handles a wrap, e.g. 3 - 250 = 9.
    uint8_t change = total - _taken;
    _taken = total;
    return change;
  }

private:
  volatile uint8_t _total = 0;   // free-running, may wrap
  uint8_t _taken = 0;            // _total at the last take()
};
