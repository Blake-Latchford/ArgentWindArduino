#pragma once

// Power saving: peripherals and pins the firmware doesn't use. (The 8MHz -> 2MHz clock prescale is in
// ClockPrescale.cpp and runs before main(), so F_CPU is 2MHz for all code.)
class DisableUnusedHardware {
 public:
  DisableUnusedHardware() = delete;

  // Turn off the peripherals the firmware doesn't use, and park unused pins.
  static void begin();
};
