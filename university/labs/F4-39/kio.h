// kio.h - DR404 lab kernel: the few services every file needs (64-bit, freestanding).
// No exceptions, no RTTI, no standard library, no heap. Paging maps virtual = physical.
#pragma once
#include <stdint.h>
#include <stddef.h>

inline void outb(uint16_t port, uint8_t v) { asm volatile("outb %0, %1" : : "a"(v), "Nd"(port)); }
inline uint8_t inb(uint16_t port)
{
    uint8_t v;
    asm volatile("inb %1, %0" : "=a"(v) : "Nd"(port));
    return v;
}
inline uint64_t rdtsc()
{
    uint32_t lo, hi;
    asm volatile("rdtsc" : "=a"(lo), "=d"(hi));
    return (static_cast<uint64_t>(hi) << 32) | lo;
}
inline uint64_t rdmsr(uint32_t msr)
{
    uint32_t lo, hi;
    asm volatile("rdmsr" : "=a"(lo), "=d"(hi) : "c"(msr));
    return (static_cast<uint64_t>(hi) << 32) | lo;
}
inline void wrmsr(uint32_t msr, uint64_t v)
{
    asm volatile("wrmsr" : : "c"(msr), "a"(static_cast<uint32_t>(v)),
                 "d"(static_cast<uint32_t>(v >> 32)));
}
struct CpuidRegs { uint32_t eax, ebx, ecx, edx; };
inline CpuidRegs cpuid(uint32_t leaf, uint32_t subleaf = 0)
{
    CpuidRegs r;
    asm volatile("cpuid" : "=a"(r.eax), "=b"(r.ebx), "=c"(r.ecx), "=d"(r.edx)
                 : "a"(leaf), "c"(subleaf));
    return r;
}

void serial_init();                        // COM1 at 0x3F8, polled
void kputs(const char* s);
void kprintf(const char* fmt, ...);        // %s %c %d %u %x %lu %lx %%; width, '0', '-'
[[noreturn]] void qemu_exit(uint8_t code); // isa-debug-exit at 0xF4: QEMU exits (code<<1)|1
[[noreturn]] void halt_forever();

// Multiboot command line ("test=hlt seconds=3"): value of key, or nullptr.
void cmdline_init(uint32_t magic, uint32_t info);
const char* cmdline();
bool arg_is(const char* key, const char* value);
uint64_t arg_num(const char* key, uint64_t fallback);

extern "C" void* memcpy(void* d, const void* s, size_t n);
extern "C" void* memset(void* d, int c, size_t n);
extern "C" int memcmp(const void* a, const void* b, size_t n);
size_t kstrlen(const char* s);
