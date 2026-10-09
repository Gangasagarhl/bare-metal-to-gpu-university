// host/k4.h (host shim) - lets the kernel driver edu4.cc (F4-17) compile and run on the build host.
// Every MMIO access goes to the device model selected by the test (edu_model.h); the
// time-stamp counter is a fake clock that advances by 1,000 ticks per read, so timeouts can be
// tested deterministically.
#pragma once
#include <cstdint>
#include <cstdio>

namespace k4 {
uint32_t mmio_read32(uintptr_t a);
void mmio_write32(uintptr_t a, uint32_t v);
uint64_t rdtsc();
inline void putc(char c) { std::putchar(c); }
inline void puts(const char* s) { std::fputs(s, stdout); }
inline void line(const char* s) { std::puts(s); }
inline void hex(uint64_t v, int digits) { std::printf("%0*llx", digits, static_cast<unsigned long long>(v)); }
inline void dec(uint64_t v) { std::printf("%llu", static_cast<unsigned long long>(v)); }
} // namespace k4
