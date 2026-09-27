#include "VaneAdc.h"

#include <Arduino.h>
#include <avr/power.h>
#include <util/delay.h>

#define VANE_DRIVE_PIN 4     // D4, top of the divider
#define VANE_ADC_CHANNEL 0   // A0, divider midpoint

void VaneAdc::begin() {
  pinMode(VANE_DRIVE_PIN, OUTPUT);
  digitalWrite(VANE_DRIVE_PIN, LOW);
  DIDR0 = _BV(ADC0D);   // no digital input buffer on the analog pin
}

uint16_t VaneAdc::sample() {
  digitalWrite(VANE_DRIVE_PIN, HIGH);
  power_adc_enable();
  ADMUX = _BV(REFS0) | VANE_ADC_CHANNEL;   // AVCC reference
  ADCSRA = _BV(ADEN) | _BV(ADPS2);         // 2MHz / 16 = 125kHz ADC clock
  _delay_us(50);

  uint16_t counts = 0;
  for (uint8_t i = 0; i < 2; i++) {  // discard the first conversion after enabling
    ADCSRA |= _BV(ADSC);
    while (ADCSRA & _BV(ADSC)) {
    }
    counts = ADC;
  }

  ADCSRA = 0;
  power_adc_disable();
  digitalWrite(VANE_DRIVE_PIN, LOW);
  return counts;
}
