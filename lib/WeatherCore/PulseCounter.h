#pragma once

#include <stdint.h>

class PulseCounter {
public:
  // Assumed called from non-reentrant ISRs.
  void increment() { _total = _total + 1; }

  // From the main loop: closures since the last call.
  // 255 counts per second would be something like mach 0.5
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
