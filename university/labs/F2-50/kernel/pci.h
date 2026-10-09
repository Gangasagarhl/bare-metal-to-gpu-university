// pci.h: PCI configuration reads through the legacy x86 I/O ports 0xCF8 (address) and
// 0xCFC (data). The address format below is written from memory and NOT verified against
// the PCI Local Bus Specification in this build (chapter F2-50, unverified box); the lab run
// shows that it found the device QEMU reports in "info pci".
#pragma once
#include <stdint.h>

namespace pci {

inline void outl(uint16_t port, uint32_t value)
{
    asm volatile("outl %0, %1" : : "a"(value), "Nd"(port));
}

inline uint32_t inl(uint16_t port)
{
    uint32_t value;
    asm volatile("inl %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

inline uint32_t read32(uint32_t bus, uint32_t dev, uint32_t fn, uint32_t offset)
{
    const uint32_t address = 0x80000000u | (bus << 16) | (dev << 11) | (fn << 8) | (offset & 0xfcu);
    outl(0xcf8, address);
    return inl(0xcfc);
}

}  // namespace pci
