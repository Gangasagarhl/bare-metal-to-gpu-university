// cov.cc - the coverage bitmap, shared across fork() so the parent fuzzer can
// read the edges that a forked child ran before it crashed or exited.
// Compiled WITHOUT -fsanitize-coverage, so this file adds no edges of its own.
#include "fuzz.h"
#include <sys/mman.h>

unsigned char* g_cov = nullptr;
const unsigned g_cov_size = 1u << 16;   // 65536 edge buckets

void cov_init()
{
    g_cov = static_cast<unsigned char*>(
        mmap(nullptr, g_cov_size, PROT_READ | PROT_WRITE,
             MAP_SHARED | MAP_ANONYMOUS, -1, 0));
}

// The compiler inserts a call to this before every edge of an instrumented
// target. We hash the return address into the shared bitmap. It is a coarse
// edge table (collisions are possible), exactly like AFL's classic bitmap.
extern "C" void __sanitizer_cov_trace_pc()
{
    if (g_cov == nullptr) return;
    auto pc = reinterpret_cast<std::uintptr_t>(__builtin_return_address(0));
    g_cov[(pc >> 1) & (g_cov_size - 1)] = 1;
}
