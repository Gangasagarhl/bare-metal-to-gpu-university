// runtime.cpp: the few functions a freestanding C++ program must supply itself, because the
// compiler emits calls to them but no library is linked (chapter F2-48, Layer 2).
#include "console.h"

#include <stddef.h>
#include <stdint.h>

extern "C" {

// The compiler may turn structure copies and zero-initialisation into calls to these.
void* memcpy(void* dst, const void* src, size_t n)
{
    auto* d = static_cast<unsigned char*>(dst);
    const auto* s = static_cast<const unsigned char*>(src);
    for (size_t i = 0; i < n; ++i) {
        d[i] = s[i];
    }
    return dst;
}

void* memset(void* dst, int value, size_t n)
{
    auto* d = static_cast<unsigned char*>(dst);
    for (size_t i = 0; i < n; ++i) {
        d[i] = static_cast<unsigned char>(value);
    }
    return dst;
}

// Called if a pure virtual function is ever called (a bug): stop loudly.
void __cxa_pure_virtual()
{
    print("PANIC: pure virtual function called\n");
    qemuExit(0x7f);
}

// Global objects with destructors register them here. A kernel never "exits", so we only
// count the registrations and never run them.
void* __dso_handle = nullptr;
int g_atexit_registrations = 0;
int __cxa_atexit(void (*)(void*), void*, void*)
{
    ++g_atexit_registrations;
    return 0;
}

}  // extern "C"

// operator new and delete backed by a tiny bump allocator in a static arena. Freed memory
// is never reused: enough for a lab, not for a kernel heap (that is milestone B5).
namespace {
alignas(16) unsigned char g_arena[4096];
size_t g_used = 0;
}  // namespace

void* operator new(size_t size)
{
    const size_t aligned = (size + 15) & ~static_cast<size_t>(15);
    if (aligned > sizeof(g_arena) - g_used) {
        print("PANIC: out of memory in operator new\n");
        qemuExit(0x7e);
    }
    void* p = g_arena + g_used;
    g_used += aligned;
    return p;
}

void operator delete(void*) noexcept {}
void operator delete(void*, size_t) noexcept {}
