#pragma once

// Hang recovery: the watchdog in interrupt-then-reset mode with a 2s timeout, so the interrupt fires at
// 2s and the reset at 4s unless the main loop re-arms it.
class Watchdog {
 public:
  Watchdog() = delete;

  // Whether the previous reset was the watchdog. Call once, before begin().
  static bool causedLastReset();

  static void begin();

  // Call on every tick.
  static void rearm();
};
