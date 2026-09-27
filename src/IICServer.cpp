#include "IICServer.h"

#include <Arduino.h>
#include <Wire.h>
#include <util/atomic.h>

// 1 (the default) when the bus has its own pull-ups on SDA/SCL, so the internal ones are turned off. 0
// keeps the internal pull-ups on; at 20-50k they're only enough for a short bus at 100kHz.
#ifndef WEATHER_I2C_EXTERNAL_PULLUPS
#define WEATHER_I2C_EXTERNAL_PULLUPS 1
#endif

WeatherRegisters IICServer::served;
volatile uint8_t IICServer::pointer;

void IICServer::begin(uint8_t address) {
  Wire.begin(address);
#if WEATHER_I2C_EXTERNAL_PULLUPS
  // Wire.begin() turns on the internal pull-ups on SDA/SCL; the bus has its own.
  PORTC &= ~(_BV(PC4) | _BV(PC5));
#endif
  Wire.onReceive(onReceive);
  Wire.onRequest(onRequest);
}

void IICServer::update(const WeatherRegisters& registers) {
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE) { served = registers; }
}

void IICServer::onReceive(int count) {
  if (count > 0) {
    pointer = Wire.read();
  }
  while (Wire.available()) {
    Wire.read();  // no writable registers
  }
}

// Runs in the TWI ISR, so the copy can't interleave with update(), and the whole read comes from this
// one copy.
void IICServer::onRequest() {
  if (pointer < sizeof(served)) {
    Wire.write((const uint8_t*)&served + pointer, sizeof(served) - pointer);
  } else {
    Wire.write(0xFF);
  }
}
