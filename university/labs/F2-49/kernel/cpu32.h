// cpu32.h: privileged-instruction wrappers for the 32-bit lab kernel (ring 0 only).
// One instruction per wrapper; every operand and side effect is declared to the compiler.
#pragma once
#include <stdint.h>

namespace cpu {

inline uint32_t readCr0()
{
    uint32_t value;
    asm volatile("mov %%cr0, %0" : "=r"(value));
    return value;
}

inline uint32_t readEflags()
{
    uint32_t value;
    asm volatile("pushfl\n\t"
                 "popl %0"
                 : "=r"(value)
                 :
                 : "memory");   // it uses the stack
    return value;
}

// Disable maskable interrupts. "memory" keeps the compiler from moving memory accesses
// across it: code after cli() often relies on not being interrupted.
inline void cli()
{
    asm volatile("cli" : : : "memory");
}

inline uint8_t inb(uint16_t port)
{
    uint8_t value;
    asm volatile("inb %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

}  // namespace cpu
