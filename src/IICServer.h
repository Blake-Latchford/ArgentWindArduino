#pragma once

#include <stdint.h>

#include "WeatherRegisters.h"

// I2C slave serving WeatherRegisters.
class IICServer {
 public:
  IICServer() = delete;

  // Joins the bus at the primary address, or at the secondary address if the address jumper is fitted.
  static void begin();

  // Replace the served values. Copies with interrupts off, so a read never sees half an update.
  static void update(const WeatherRegisters& registers);

 private:
  static bool addressJumperFitted();

  // Wire.onReceive() takes a void (*)(int), so the parameter is int rather than a sized type.
  static void onReceive(int count);
  static void onRequest();

  // Read by the TWI ISR, written by update() with interrupts off.
  static WeatherRegisters served;
  static volatile uint8_t pointer;
};
