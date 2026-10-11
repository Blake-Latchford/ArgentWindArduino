#pragma once

// Force-included into every file of the build, the Arduino core's included (platformio.ini). The core
// compiles its USB stack whenever the USBCON register is defined, and that stack rejects a 2MHz F_CPU.
// Undefining the name leaves the stack out, and with it the USBDevice.attach() in the core's main(). The
// firmware doesn't use USB. avr/io.h has an include guard, so later includes don't define USBCON again.
#include <avr/io.h>
#undef USBCON

// USBCON under a name the core doesn't check, so DisableUnusedHardware can turn USB off.
#define USB_CONTROL _SFR_MEM8(0xD8)
