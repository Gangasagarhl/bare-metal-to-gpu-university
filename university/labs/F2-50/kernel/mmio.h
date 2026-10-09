// mmio.h: a typed wrapper for one memory-mapped device register.
// Every access goes through a volatile pointer of exactly the register's width, so the
// compiler must perform each read and write, in program order, with that width
// (chapter F2-50). volatile does NOT order accesses against other CPUs or DMA: that needs
// barriers, which this single-CPU, uncached lab does not use.
#pragma once
#include <stdint.h>

enum class Access { ReadOnly, WriteOnly, ReadWrite };

template <typename T, uint32_t Offset, Access A = Access::ReadWrite>
struct Reg {
    static_assert(sizeof(T) == 1 || sizeof(T) == 2 || sizeof(T) == 4, "register width");
    static_assert(Offset % sizeof(T) == 0, "registers are naturally aligned");
    static constexpr uint32_t offset = Offset;

    static T read(uintptr_t base)
    {
        static_assert(A != Access::WriteOnly, "this register is write-only");
        return *reinterpret_cast<volatile const T*>(base + Offset);
    }
    static void write(uintptr_t base, T value)
    {
        static_assert(A != Access::ReadOnly, "this register is read-only");
        *reinterpret_cast<volatile T*>(base + Offset) = value;
    }
};
