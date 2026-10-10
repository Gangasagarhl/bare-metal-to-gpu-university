// portbus.h - DR301: the x86 I/O-port bus for reg.h (kernel only: it uses IN/OUT).
#pragma once
#include "kbase.h"

struct PortBus {
    uint16_t base;
    template <typename T> T read(uint32_t off) const
    {
        const auto p = static_cast<uint16_t>(base + off);
        if constexpr (sizeof(T) == 1) return inb(p);
        else if constexpr (sizeof(T) == 2) return inw(p);
        else return inl(p);
    }
    template <typename T> void write(uint32_t off, T v) const
    {
        const auto p = static_cast<uint16_t>(base + off);
        if constexpr (sizeof(T) == 1) outb(p, v);
        else if constexpr (sizeof(T) == 2) outw(p, v);
        else outl(p, v);
    }
};
