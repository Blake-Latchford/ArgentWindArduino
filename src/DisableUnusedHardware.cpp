#include "DisableUnusedHardware.h"

#include <Arduino.h>
#include <avr/power.h>

void DisableUnusedHardware::begin() {
  ADCSRA = 0;
  ACSR = _BV(ACD);        // analog comparator off
  power_adc_disable();    // VaneAdc powers it up only while sampling
  power_spi_disable();
  power_timer0_disable(); // millis()/delay() stop; the 1Hz tick comes from Timer1
  power_timer2_disable();
#ifndef WEATHER_DEBUG
  power_usart0_disable();
#endif

  // Unused pins: inputs with pull-ups so they don't float. D13 drives the onboard LED, so hold it low.
  const uint8_t unused_pins[] = {5, 6, 7, 8, 9, 10, 11, 12, A1, A2, A3};
  for (uint8_t pin : unused_pins) {
    pinMode(pin, INPUT_PULLUP);
  }
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);
}
