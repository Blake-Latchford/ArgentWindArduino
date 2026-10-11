#pragma once

#include <Arduino.h>

// Pin-change interrupts on one port, so one ISR covers both.
// PORTB is the only such port on the 32U4, and D2/D3 are its I2C.
#define BOARD_ANEMOMETER_PIN 8
#define BOARD_RAIN_PIN 9

#define BOARD_VANE_DRIVE_PIN 4
#define BOARD_VANE_ADC_PIN A0

// Adjacent, so a 2-pin shunt fits; no free pin sits next to GND, so the second pin is driven low.
#define BOARD_ADDRESS_JUMPER_SENSE_PIN 5
#define BOARD_ADDRESS_JUMPER_GROUND_PIN 6

#if defined(__AVR_ATmega32U4__)
// The RX/TX LEDs are lit by a low pin. Serial is USB here, so debug goes to the UART.
#define BOARD_LEDS {LED_BUILTIN_RX, HIGH}, {LED_BUILTIN_TX, HIGH}
#define BOARD_DEBUG_SERIAL Serial1
#else
#define BOARD_LEDS {LED_BUILTIN, LOW}
#define BOARD_DEBUG_SERIAL Serial
#endif
