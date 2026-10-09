// hid_kbd.h - DR302 F4-09: a HID boot-protocol keyboard driver.
// Boot protocol, SET_PROTOCOL and SET_IDLE: Device Class Definition for HID ("Boot
// Interface Descriptors", "Class-Specific Requests"); key usages: HID Usage Tables
// (Keyboard/Keypad page). From memory, pending verification.
#pragma once
#include "usbh.h"

namespace hidkbd {
constexpr uint8_t REQ_SET_IDLE = 0x0A, REQ_SET_PROTOCOL = 0x0B;
bool start(usbh::Device& d);                       // boot protocol, idle 0, endpoint ready
// Collects key presses until Enter (or timeout); returns the number of characters.
int read_line(usbh::Device& d, char* out, int max, uint32_t timeout_ms);
char usage_to_ascii(uint8_t usage, bool shift);
}
