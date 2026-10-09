// pl011.h - F4-24: PL011-specific calls beyond F3-18's serial.h interface.
#pragma once
#include <cstdint>

namespace pl011 {
void set_base(uint64_t phys);   // from the devicetree's stdout-path node
uint64_t base();
bool read_char(char* c);        // polled receive; false if the FIFO is empty
void enable_rx_interrupt();     // used by F4-25's interrupt test
}
