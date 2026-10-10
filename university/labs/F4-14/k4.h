// k4.h - DR401 lab kernel base: port I/O, MMIO, the COM1 console, printing, CPUID, TSC, exit.
// Freestanding C++20 (no exceptions, no RTTI, no standard library beyond <stdint.h>).
#pragma once
#include <stdint.h>
#include <stddef.h>

namespace k4 {

inline void outb(uint16_t port, uint8_t v) { asm volatile("outb %0, %1" : : "a"(v), "Nd"(port)); }
inline void outw(uint16_t port, uint16_t v) { asm volatile("outw %0, %1" : : "a"(v), "Nd"(port)); }
inline void outl(uint16_t port, uint32_t v) { asm volatile("outl %0, %1" : : "a"(v), "Nd"(port)); }
inline uint8_t inb(uint16_t port) { uint8_t v; asm volatile("inb %1, %0" : "=a"(v) : "Nd"(port)); return v; }
inline uint32_t inl(uint16_t port) { uint32_t v; asm volatile("inl %1, %0" : "=a"(v) : "Nd"(port)); return v; }

// MMIO: one volatile access of exactly the named width (paging is off: address = physical).
inline uint32_t mmio_read32(uintptr_t a) { return *reinterpret_cast<volatile uint32_t*>(a); }
inline void mmio_write32(uintptr_t a, uint32_t v) { *reinterpret_cast<volatile uint32_t*>(a) = v; }

inline uint64_t rdtsc()
{
    uint32_t lo, hi;
    asm volatile("rdtsc" : "=a"(lo), "=d"(hi));
    return (static_cast<uint64_t>(hi) << 32) | lo;
}

struct Cpuid { uint32_t a, b, c, d; };
inline Cpuid cpuid(uint32_t leaf, uint32_t sub = 0)
{
    Cpuid r;
    asm volatile("cpuid" : "=a"(r.a), "=b"(r.b), "=c"(r.c), "=d"(r.d) : "a"(leaf), "c"(sub));
    return r;
}

void console_init();                  // COM1, 8 data bits, no parity, 1 stop bit
void putc(char c);
void puts(const char* s);             // no newline added
void hex(uint64_t v, int digits);     // fixed number of hex digits, no "0x"
void dec(uint64_t v);
void line(const char* s);             // puts + newline
[[noreturn]] void exit_qemu(uint8_t code);   // isa-debug-exit: QEMU exits with (code << 1) | 1
[[noreturn]] void panic(const char* why);    // prints "PANIC: why" and exits with code 0x7f

uint64_t udiv64(uint64_t n, uint64_t d);     // 32-bit kernel: no libgcc, so 64-bit division here

} // namespace k4
