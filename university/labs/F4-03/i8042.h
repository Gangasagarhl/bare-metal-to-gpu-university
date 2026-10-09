// i8042.h - DR301 F4-03: the PS/2 controller, keyboard and mouse.
#pragma once
#include <stdint.h>

struct KeyEvent { uint8_t code; bool pressed; char ascii; };   // ascii 0: not printable
struct MouseEvent { int dx, dy; uint8_t buttons; };           // buttons: bit 0 L, 1 R, 2 M

namespace ps2 {
int init();                              // 0 or a negative error; logs every step
bool get_key(KeyEvent& e);
bool get_mouse(MouseEvent& e);
uint32_t resyncs();                      // mouse bytes thrown away to find a packet start
}
