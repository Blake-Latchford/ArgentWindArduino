#pragma once

class DisableUnusedHardware {
 public:
  DisableUnusedHardware() = delete;

  // Call before the other drivers' begin(), which claim their own pins.
  static void begin();
};
