#include "IICServer.h"

#include <Arduino.h>
#include <Wire.h>
#include <util/atomic.h>
#include <util/delay.h>

#include "Board.h"

// The address jumper selects between these at reset (platformio.ini).
#ifndef WEATHER_I2C_PRIMARY_ADDRESS
#define WEATHER_I2C_PRIMARY_ADDRESS 0x6C
#endif
#ifndef WEATHER_I2C_SECONDARY_ADDRESS
#define WEATHER_I2C_SECONDARY_ADDRESS 0x6E
#endif

WeatherRegisters IICServer::served;
volatile uint8_t IICServer::pointer = 0;

void IICServer::begin() {
  Wire.begin(addressJumperFitted() ? WEATHER_I2C_SECONDARY_ADDRESS : WEATHER_I2C_PRIMARY_ADDRESS);
  // Wire.begin() turns on the internal pull-ups on SDA/SCL; the bus has its own. On an input, writing
  // LOW turns the pull-up off.
  digitalWrite(SDA, LOW);
  digitalWrite(SCL, LOW);
  Wire.onReceive(onReceive);
  Wire.onRequest(onRequest);
}

bool IICServer::addressJumperFitted() {
  pinMode(BOARD_ADDRESS_JUMPER_GROUND_PIN, OUTPUT);
  digitalWrite(BOARD_ADDRESS_JUMPER_GROUND_PIN, LOW);
  pinMode(BOARD_ADDRESS_JUMPER_SENSE_PIN, INPUT_PULLUP);
  _delay_us(100);  // let the pull-up charge the pin and any wiring
  bool fitted = digitalRead(BOARD_ADDRESS_JUMPER_SENSE_PIN) == LOW;
  pinMode(BOARD_ADDRESS_JUMPER_GROUND_PIN, INPUT_PULLUP);
  return fitted;
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
// one copy. Wire doesn't report how many bytes the master took, so every read returns the pointer to
// 0x00, and a plain read with no pointer write gets the whole map.
void IICServer::onRequest() {
  if (pointer < sizeof(served)) {
    Wire.write((const uint8_t*)&served + pointer, sizeof(served) - pointer);
  } else {
    Wire.write(0xFF);
  }
  pointer = 0;
}
