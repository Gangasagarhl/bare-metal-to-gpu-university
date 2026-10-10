// serial.h - F3-18: polled output on the first PC serial port (COM1, a 16550-compatible UART).
#pragma once

namespace serial {
void init();
void put(char c);
void write(const char* s);
}
