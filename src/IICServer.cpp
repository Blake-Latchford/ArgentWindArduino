#include "IICServer.h"

#include <Arduino.h>
#include <Wire.h>
#include <util/atomic.h>
#include <util/delay.h>

// 1 (the default) when the bus has its own pull-ups on SDA/SCL, so the internal ones are turned off. 0
// keeps the internal pull-ups on; at 20-50k they're only enough for a short bus at 100kHz.
#ifndef WEATHER_I2C_EXTERNAL_PULLUPS
#define WEATHER_I2C_EXTERNAL_PULLUPS 1
#endif

// A shunt across D5 and D6 selects the secondary address. D6 is held low as the jumper's ground, so the
// jumper fits a 2-pin header on adjacent pins.
#define ADDRESS_JUMPER_SENSE_PIN 5
#define ADDRESS_JUMPER_GROUND_PIN 6

WeatherRegisters IICServer::served;
volatile uint8_t IICServer::pointer;

void IICServer::begin(uint8_t address) {
  Wire.begin(addressJumperFitted() ? WEATHER_I2C_SECONDARY_ADDRESS : address);
#if WEATHER_I2C_EXTERNAL_PULLUPS
  // Wire.begin() turns on the internal pull-ups on SDA/SCL; the bus has its own.
  PORTC &= ~(_BV(PC4) | _BV(PC5));
#endif
  Wire.onReceive(onReceive);
  Wire.onRequest(onRequest);
}

// Read once at boot; moving the jumper takes effect on the next reset.
bool IICServer::addressJumperFitted() {
  pinMode(ADDRESS_JUMPER_GROUND_PIN, OUTPUT);
  digitalWrite(ADDRESS_JUMPER_GROUND_PIN, LOW);
  pinMode(ADDRESS_JUMPER_SENSE_PIN, INPUT_PULLUP);
  _delay_us(100);  // let the pull-up charge the pin and any wiring
  if (digitalRead(ADDRESS_JUMPER_SENSE_PIN) == HIGH) {
    return false;
  }
  // The jumper holds the pin low, so turn the pull-up off rather than let it draw ~100uA through the
  // jumper.
  pinMode(ADDRESS_JUMPER_SENSE_PIN, INPUT);
  return true;
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
