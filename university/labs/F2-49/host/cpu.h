// cpu.h: small inline-assembly wrappers for x86-64 user mode. Each wrapper is one
// instruction, states every input, output and clobber, and has a C++ type.
#pragma once
#include <array>
#include <cstdint>
#include <string>

namespace cpu {

struct CpuidResult {
    std::uint32_t eax, ebx, ecx, edx;
};

// CPUID: leaf in eax (and subleaf in ecx) in; four registers out.
inline CpuidResult cpuid(std::uint32_t leaf, std::uint32_t subleaf = 0)
{
    CpuidResult r{};
    asm volatile("cpuid"
                 : "=a"(r.eax), "=b"(r.ebx), "=c"(r.ecx), "=d"(r.edx)
                 : "a"(leaf), "c"(subleaf));
    return r;
}

// The 12-character vendor string of leaf 0, stored in ebx, edx, ecx (in that order).
inline std::string vendor()
{
    const CpuidResult r = cpuid(0);
    std::array<char, 12> s{};
    for (int i = 0; i < 4; ++i) {
        s[static_cast<std::size_t>(i)] = static_cast<char>(r.ebx >> (8 * i));
        s[static_cast<std::size_t>(4 + i)] = static_cast<char>(r.edx >> (8 * i));
        s[static_cast<std::size_t>(8 + i)] = static_cast<char>(r.ecx >> (8 * i));
    }
    return std::string(s.begin(), s.end());
}

// RDTSC: the time-stamp counter, returned in edx:eax. "volatile" because two calls with
// the same (no) inputs must still both execute and may return different values.
inline std::uint64_t rdtsc()
{
    std::uint32_t lo = 0, hi = 0;
    asm volatile("rdtsc" : "=a"(lo), "=d"(hi));
    return (static_cast<std::uint64_t>(hi) << 32) | lo;
}

// BSR: index of the highest set bit of a non-zero value. "cc": it changes the flags.
inline unsigned highestBit(std::uint64_t value)
{
    std::uint64_t index = 0;
    asm("bsrq %1, %0" : "=r"(index) : "rm"(value) : "cc");
    return static_cast<unsigned>(index);
}

// A compiler barrier: emits no instruction, but the "memory" clobber tells the compiler
// that memory may have changed, so it must not keep values in registers across it.
inline void compilerBarrier()
{
    asm volatile("" : : : "memory");
}

}  // namespace cpu
