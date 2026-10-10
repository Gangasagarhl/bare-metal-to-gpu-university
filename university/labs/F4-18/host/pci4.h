// host/pci4.h (host shim) - the parts of the kernel's PCI interface that edu4.cc uses, answered
// from a fake configuration space (one function, 1234:11e8, one 1 MiB memory BAR).
#pragma once
#include <cstdint>

namespace pci4 {
struct Addr { uint8_t bus, dev, fn; };
struct Function {
    Addr at;
    uint16_t vendor, device, subvendor, subdevice;
    uint8_t revision, base_class, sub_class, prog_if, header_type;
};
struct Bar { uint32_t base; uint32_t size; bool io; bool is64; };
uint16_t read16(Addr a, uint8_t off);
void write32(Addr a, uint8_t off, uint32_t v);
Bar read_bar(Addr a, int i);
} // namespace pci4
