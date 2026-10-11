#pragma once

#include <Arduino.h>

// Pro Micro pin map: the SparkFun Qwiic Pro Micro and the 3.3V Pro Micro share the ATmega32U4 and its
// pinout. I2C is fixed by the chip on D2 (SDA) and D3 (SCL), also on the Qwiic port.
#if !defined(__AVR_ATmega32U4__)
#error "This firmware targets the ATmega32U4 (Pro Micro)"
#endif

// Pin-change interrupts on one port, so one ISR covers both. PORTB is the only such port on the 32U4.
#define BOARD_ANEMOMETER_PIN 8
#define BOARD_RAIN_PIN 9

#define BOARD_VANE_DRIVE_PIN 4
#define BOARD_VANE_ADC_PIN A0

// Adjacent, so a 2-pin shunt fits; no free pin sits next to GND, so the second pin is driven low.
#define BOARD_ADDRESS_JUMPER_SENSE_PIN 5
#define BOARD_ADDRESS_JUMPER_GROUND_PIN 6

// The RX/TX LEDs are lit by a low pin. Serial is USB here, so debug goes to the UART.
#define BOARD_LEDS {LED_BUILTIN_RX, HIGH}, {LED_BUILTIN_TX, HIGH}
#define BOARD_DEBUG_SERIAL Serial1
