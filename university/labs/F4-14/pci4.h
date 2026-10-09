// pci4.h - PCI configuration access (configuration mechanism #1, I/O ports 0xCF8/0xCFC)
// and an enumerator that visits every function of every device on every bus.
#pragma once
#include <stdint.h>

namespace pci4 {

struct Addr { uint8_t bus, dev, fn; };

struct Function {
    Addr at;
    uint16_t vendor, device, subvendor, subdevice;
    uint8_t revision, base_class, sub_class, prog_if, header_type;
};

uint32_t read32(Addr a, uint8_t off);
void write32(Addr a, uint8_t off, uint32_t v);
uint16_t read16(Addr a, uint8_t off);
uint8_t read8(Addr a, uint8_t off);
bool read_function(Addr a, Function& f);           // false: no function at this address

// Calls visit(f, ctx) for each function found. Returns the number of functions.
using Visitor = void (*)(const Function& f, void* ctx);
int enumerate(Visitor visit, void* ctx);

// BAR i of a type-0 header: base address and (by the standard size probe) its size.
struct Bar { uint32_t base; uint32_t size; bool io; bool is64; };
Bar read_bar(Addr a, int i);

} // namespace pci4
