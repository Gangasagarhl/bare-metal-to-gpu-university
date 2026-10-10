// uart16550.h - F4-28: 16550-specific calls beyond F3-18's serial.h interface.
#pragma once
#include <cstdint>

namespace uart16550 {
void set_base(uint64_t phys, uint32_t reg_shift);   // from the devicetree
uint64_t base();
bool read_char(char* c);
void enable_rx_interrupt();                         // F4-29: PLIC test
}
