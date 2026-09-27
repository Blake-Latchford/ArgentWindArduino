#pragma once

#include <stdint.h>

#include "WeatherRegisters.h"

// I2C slave serving WeatherRegisters. The master writes one byte to set the register pointer, then reads
// from there to the end of the map.
class IICServer {
 public:
  IICServer() = delete;

  static void begin(uint8_t address);

  // Replace the served values. Copies with interrupts off, so a read never sees half an update.
  static void update(const WeatherRegisters& registers);

 private:
  // Wire.onReceive() takes a void (*)(int), so the parameter is int rather than a sized type.
  static void onReceive(int count);
  static void onRequest();

  // Read by the TWI ISR, written by update() with interrupts off.
  static WeatherRegisters served;
  static volatile uint8_t pointer;
};
