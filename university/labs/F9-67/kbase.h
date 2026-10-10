// Copied from the DR403 lab folder F4-31 (only this line added) (the course kernel base); RB403 reuses it.
// kbase.h - DR403 kernel base: MMIO access, a console that a driver plugs in, a small printf,
// the architectural counter, and a way to stop QEMU with an exit code (semihosting).
#pragma once
#include <stddef.h>
#include <stdint.h>

namespace k {

inline uint32_t rd32(uint64_t addr) { return *reinterpret_cast<volatile uint32_t*>(addr); }
inline void wr32(uint64_t addr, uint32_t v) { *reinterpret_cast<volatile uint32_t*>(addr) = v; }
inline uint16_t rd16(uint64_t addr) { return *reinterpret_cast<volatile uint16_t*>(addr); }
inline void wr16(uint64_t addr, uint16_t v) { *reinterpret_cast<volatile uint16_t*>(addr) = v; }
inline uint8_t rd8(uint64_t addr) { return *reinterpret_cast<volatile uint8_t*>(addr); }
inline void wr8(uint64_t addr, uint8_t v) { *reinterpret_cast<volatile uint8_t*>(addr) = v; }

using PutcFn = void (*)(char);
void set_console(PutcFn fn);                  // until a driver calls this, output is dropped
void printf(const char* fmt, ...) __attribute__((format(printf, 1, 2)));
[[noreturn]] void exit(int code);             // ends the QEMU run with this exit code

inline uint64_t counter_freq()
{
    uint64_t v;
    asm volatile("mrs %0, cntfrq_el0" : "=r"(v));
    return v;
}
inline uint64_t counter()
{
    uint64_t v;
    asm volatile("isb; mrs %0, cntpct_el0" : "=r"(v));
    return v;
}
void delay_us(uint64_t us);                   // busy wait on the architectural counter

}  // namespace k

extern "C" void kmain(const void* dtb, uint64_t entry_el, uint64_t load_addr);
