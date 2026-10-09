// arch.h - F3-18: the few x86-64 instructions the kernel needs, as small inline functions.
// Every inline-assembly statement in the kernel lives here or in boot.S (curriculum 1.1).
#pragma once
#include <cstdint>

namespace arch {

inline void outb(uint16_t port, uint8_t value)
{
    asm volatile("outb %0, %1" : : "a"(value), "Nd"(port) : "memory");
}

inline uint8_t inb(uint16_t port)
{
    uint8_t value;
    asm volatile("inb %1, %0" : "=a"(value) : "Nd"(port) : "memory");
    return value;
}

inline void outl(uint16_t port, uint32_t value)
{
    asm volatile("outl %0, %1" : : "a"(value), "Nd"(port) : "memory");
}

inline uint32_t inl(uint16_t port)
{
    uint32_t value;
    asm volatile("inl %1, %0" : "=a"(value) : "Nd"(port) : "memory");
    return value;
}

inline void cli() { asm volatile("cli" : : : "memory"); }
inline void sti() { asm volatile("sti" : : : "memory"); }
inline void hlt() { asm volatile("hlt" : : : "memory"); }
// sti takes effect after the next instruction, so no interrupt can slip in between the two:
// waiting with "sti; hlt" cannot miss a wake-up that arrives just before the hlt (F3-25).
inline void sti_hlt() { asm volatile("sti; hlt" : : : "memory"); }
inline void pause() { asm volatile("pause" : : : "memory"); }   // spin-wait hint
// Interrupt flag save and restore for locks (F3-23) and the timer queue (F3-25).
inline uint64_t save_flags_cli()
{
    uint64_t flags;
    asm volatile("pushfq\n\tpopq %0\n\tcli" : "=r"(flags) : : "memory");
    return flags;
}
inline void restore_flags(uint64_t flags)
{
    if (flags & (uint64_t{1} << 9)) {   // IF was set before: enable interrupts again
        sti();
    }
}

[[noreturn]] inline void halt_forever()
{
    for (;;) {
        asm volatile("cli; hlt" : : : "memory");
    }
}

inline uint64_t read_cr0() { uint64_t v; asm volatile("mov %%cr0, %0" : "=r"(v)); return v; }
inline uint64_t read_cr2() { uint64_t v; asm volatile("mov %%cr2, %0" : "=r"(v)); return v; }
inline uint64_t read_cr3() { uint64_t v; asm volatile("mov %%cr3, %0" : "=r"(v)); return v; }
inline uint64_t read_cr4() { uint64_t v; asm volatile("mov %%cr4, %0" : "=r"(v)); return v; }
inline void write_cr0(uint64_t v) { asm volatile("mov %0, %%cr0" : : "r"(v) : "memory"); }
inline void write_cr3(uint64_t v) { asm volatile("mov %0, %%cr3" : : "r"(v) : "memory"); }
inline void write_cr4(uint64_t v) { asm volatile("mov %0, %%cr4" : : "r"(v) : "memory"); }

inline uint64_t rdmsr(uint32_t msr)
{
    uint32_t lo, hi;
    asm volatile("rdmsr" : "=a"(lo), "=d"(hi) : "c"(msr));
    return (uint64_t(hi) << 32) | lo;
}

inline void wrmsr(uint32_t msr, uint64_t value)
{
    asm volatile("wrmsr" : : "c"(msr), "a"(uint32_t(value)), "d"(uint32_t(value >> 32)) : "memory");
}

inline uint64_t rdtsc()
{
    uint32_t lo, hi;
    asm volatile("rdtsc" : "=a"(lo), "=d"(hi));
    return (uint64_t(hi) << 32) | lo;
}

inline void cpuid(uint32_t leaf, uint32_t sub, uint32_t& a, uint32_t& b, uint32_t& c, uint32_t& d)
{
    asm volatile("cpuid" : "=a"(a), "=b"(b), "=c"(c), "=d"(d) : "a"(leaf), "c"(sub));
}

inline void invlpg(uint64_t virt)
{
    asm volatile("invlpg (%0)" : : "r"(virt) : "memory");
}

// QEMU's isa-debug-exit device (iobase 0xf4): writing v ends QEMU with exit status (v << 1) | 1.
inline constexpr uint8_t kExitPass = 0x10;   // QEMU exit status 33
inline constexpr uint8_t kExitFail = 0x11;   // QEMU exit status 35

[[noreturn]] inline void qemu_exit(uint8_t code)
{
    outb(0xf4, code);
    halt_forever();   // reached only without the isa-debug-exit device
}

} // namespace arch
