// shim_host.cc - host implementations of the kernel interfaces edu4.cc calls.
#include "shim_host.h"
#include "k4.h"
#include "pci4.h"
#include <linux/pci_regs.h>

Backend* g_backend = nullptr;
uint64_t g_clock = 0;
static uint16_t g_command = 0;

namespace k4 {
uint32_t mmio_read32(uintptr_t a) { return g_backend->read(static_cast<uint32_t>(a - kFakeBar0)); }
void mmio_write32(uintptr_t a, uint32_t v) { g_backend->write(static_cast<uint32_t>(a - kFakeBar0), v); }
uint64_t rdtsc() { return g_clock += 1000; }
} // namespace k4

namespace pci4 {
uint16_t read16(Addr, uint8_t off) { return off == PCI_COMMAND ? g_command : 0; }
void write32(Addr, uint8_t off, uint32_t v) { if (off == PCI_COMMAND) g_command = static_cast<uint16_t>(v); }
Bar read_bar(Addr, int i) { return i == 0 ? Bar{kFakeBar0, 0x00100000, false, false} : Bar{0, 0, false, false}; }
} // namespace pci4
