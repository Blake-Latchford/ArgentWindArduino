#include "VaneAdc.h"

#include <Arduino.h>
#include <avr/power.h>
#include <util/delay.h>

#include "Board.h"

static uint8_t channel;

void VaneAdc::begin() {
  pinMode(BOARD_VANE_DRIVE_PIN, OUTPUT);
  digitalWrite(BOARD_VANE_DRIVE_PIN, LOW);
  pinMode(BOARD_VANE_ADC_PIN, INPUT);  // a parked pin's pull-up would load the divider

  // The 32U4's analog pins don't map to channels in order.
  channel = analogPinToChannel(BOARD_VANE_ADC_PIN - A0);

  // A digital input buffer draws current with its pin held mid-rail.
  if (channel >= 8) {
    DIDR2 |= _BV(channel - 8);
  } else {
    DIDR0 |= _BV(channel);
  }
}

uint16_t VaneAdc::sample() {
  digitalWrite(BOARD_VANE_DRIVE_PIN, HIGH);
  power_adc_enable();
  // AVCC also feeds the divider, so the reading is ratiometric
  ADMUX = _BV(REFS0) | (channel & 0x07);
  ADCSRB = (channel & 0x08) ? _BV(MUX5) : 0;
  // Enable the ADC, and set the clock for full resolution.
  ADCSRA = _BV(ADEN) | _BV(ADPS2);
  _delay_us(50);

  // Discard the first conversion since it's unreliable.
  ADCSRA |= _BV(ADSC);
  while (ADCSRA & _BV(ADSC));

  ADCSRA |= _BV(ADSC);
  while (ADCSRA & _BV(ADSC));
  const uint16_t counts = ADC;

  ADCSRA = 0;
  power_adc_disable();
  digitalWrite(BOARD_VANE_DRIVE_PIN, LOW);
  return counts;
}