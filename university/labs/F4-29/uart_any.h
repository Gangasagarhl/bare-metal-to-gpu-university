// uart_any.h - F4-29: pick the UART driver from the devicetree node's compatible string.
#pragma once
#include <cstdint>
#include "fdt.h"

namespace uart {
bool init_from(const fdt::Blob& dt, const fdt::Node& n);   // false: unknown UART
const char* kind();
uint64_t base();
bool read_char(char* c);
void enable_rx_interrupt();   // ns16550a only in this lab
}
