#include "DisableUnusedHardware.h"

#include <Arduino.h>
#include <avr/power.h>

#include "Board.h"

void DisableUnusedHardware::begin() {
  // Both analog blocks are on after reset. The ADC has to be disabled before its clock is stopped, or
  // it keeps drawing current; VaneAdc turns it on only while sampling.
  ADCSRA = 0;
  ACSR = _BV(ACD);
  power_adc_disable();
  power_spi_disable();
  power_timer0_disable();  // so millis() and delay() don't work; the 1Hz tick is Timer1
  power_timer2_disable();
#ifndef WEATHER_DEBUG
  power_usart0_disable();
#endif

  // Disable the USB hardware if present.
#if defined(UHWCON)
  UDIEN = 0;
  UDCON |= _BV(DETACH);
  USB_CONTROL = _BV(FRZCLK);
  PLLCSR = 0;
  UHWCON &= ~_BV(UVREGE);
  power_usb_disable();
#endif

  // Unused pins: inputs with pull-ups so they don't float.
  // Skip SDA/SCL in case it would set the master I2C bus to 5V.
  for (uint8_t pin = 2; pin < NUM_DIGITAL_PINS; pin++) {
    if (pin != SDA && pin != SCL) {
      pinMode(pin, INPUT_PULLUP);
    }
  }

  // Level before direction, so an LED that's off when high never flashes on.
  struct Led {
    uint8_t pin;
    uint8_t off_level;
  };
  const Led leds[] = {BOARD_LEDS};
  for (const Led& led : leds) {
    digitalWrite(led.pin, led.off_level);
    pinMode(led.pin, OUTPUT);
  }
}
