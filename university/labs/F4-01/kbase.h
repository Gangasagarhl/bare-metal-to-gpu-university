// kbase.h - DR301 lab kernel: the few services every driver file needs.
// Freestanding C++: no exceptions, no RTTI, no standard library, no heap.
#pragma once
#include <stdint.h>
#include <stddef.h>

// Port I/O (x86 IN/OUT instructions) in the three widths devices use.
inline void outb(uint16_t port, uint8_t v) { asm volatile("outb %0, %1" : : "a"(v), "Nd"(port)); }
inline void outw(uint16_t port, uint16_t v) { asm volatile("outw %0, %1" : : "a"(v), "Nd"(port)); }
inline void outl(uint16_t port, uint32_t v) { asm volatile("outl %0, %1" : : "a"(v), "Nd"(port)); }
inline uint8_t inb(uint16_t port)
{
    uint8_t v;
    asm volatile("inb %1, %0" : "=a"(v) : "Nd"(port));
    return v;
}
inline uint16_t inw(uint16_t port)
{
    uint16_t v;
    asm volatile("inw %1, %0" : "=a"(v) : "Nd"(port));
    return v;
}
inline uint32_t inl(uint16_t port)
{
    uint32_t v;
    asm volatile("inl %1, %0" : "=a"(v) : "Nd"(port));
    return v;
}

// Memory-mapped I/O: every access is volatile, so the compiler neither removes, merges
// nor reorders it with other volatile accesses. Paging is off, so physical = virtual.
template <typename T> inline T mmio_read(uintptr_t addr)
{
    return *reinterpret_cast<volatile T*>(addr);
}
template <typename T> inline void mmio_write(uintptr_t addr, T v)
{
    *reinterpret_cast<volatile T*>(addr) = v;
}
// Compiler barrier: keeps the compiler from moving ordinary memory accesses (descriptors
// in RAM) across it. x86 keeps stores in order for the CPU; other CPUs need a real fence.
inline void compiler_barrier() { asm volatile("" : : : "memory"); }

inline uint64_t rdtsc()
{
    uint32_t lo, hi;
    asm volatile("rdtsc" : "=a"(lo), "=d"(hi));
    return (static_cast<uint64_t>(hi) << 32) | lo;
}

void serial_init();                      // COM1, polled: the kernel's log channel
void serial_putc(char c);
void kputs(const char* s);
void kprintf(const char* fmt, ...);      // %s %c %d %u %x %lx(64-bit) %%; width, '0', '-'
[[noreturn]] void qemu_exit(uint8_t code); // isa-debug-exit: QEMU exits with (code << 1) | 1
[[noreturn]] void panic(const char* msg);

extern "C" void* memcpy(void* d, const void* s, size_t n);
extern "C" void* memset(void* d, int c, size_t n);
extern "C" int memcmp(const void* a, const void* b, size_t n);
size_t kstrlen(const char* s);
int kstrcmp(const char* a, const char* b);

uint32_t fnv1a(const void* data, size_t n, uint32_t h = 2166136261u);
