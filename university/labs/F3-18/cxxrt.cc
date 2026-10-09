// cxxrt.cc - F3-18: the small runtime a freestanding C++ kernel must provide itself.
#include <cstddef>
#include <cstdint>
#include "panic.h"

extern "C" {

// The compiler may emit calls to these four even in freestanding code.
void* memset(void* dst, int value, size_t n)
{
    auto* d = static_cast<unsigned char*>(dst);
    while (n-- > 0) {
        *d++ = static_cast<unsigned char>(value);
    }
    return dst;
}

void* memcpy(void* dst, const void* src, size_t n)
{
    auto* d = static_cast<unsigned char*>(dst);
    auto* s = static_cast<const unsigned char*>(src);
    while (n-- > 0) {
        *d++ = *s++;
    }
    return dst;
}

void* memmove(void* dst, const void* src, size_t n)
{
    auto* d = static_cast<unsigned char*>(dst);
    auto* s = static_cast<const unsigned char*>(src);
    if (d < s) {
        while (n-- > 0) {
            *d++ = *s++;
        }
    } else {
        while (n-- > 0) {
            d[n] = s[n];
        }
    }
    return dst;
}

int memcmp(const void* a, const void* b, size_t n)
{
    auto* x = static_cast<const unsigned char*>(a);
    auto* y = static_cast<const unsigned char*>(b);
    for (size_t i = 0; i < n; ++i) {
        if (x[i] != y[i]) {
            return x[i] < y[i] ? -1 : 1;
        }
    }
    return 0;
}

// Called if a pure virtual function is ever called (Itanium C++ ABI).
void __cxa_pure_virtual()
{
    PANIC("pure virtual function called");
}

// The kernel never exits, so destructors of global objects are never registered or run.
void* __dso_handle = nullptr;
int __cxa_atexit(void (*)(void*), void*, void*)
{
    return 0;
}

// Global constructors: the linker script collects their addresses in .init_array.
using Ctor = void (*)();
extern Ctor __init_array_start[];
extern Ctor __init_array_end[];

void run_global_constructors()
{
    for (Ctor* c = __init_array_start; c != __init_array_end; ++c) {
        (*c)();
    }
}

} // extern "C"
