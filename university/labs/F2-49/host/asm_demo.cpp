// asm_demo.cpp: uses the wrappers of cpu.h and checks them against plain C++ where possible.
#include "cpu.h"

#include <bit>
#include <cstdio>

int main()
{
    std::printf("cpuid leaf 0: highest standard leaf = %u, vendor = \"%s\"\n",
                cpu::cpuid(0).eax, cpu::vendor().c_str());

    const std::uint64_t t1 = cpu::rdtsc();
    const std::uint64_t t2 = cpu::rdtsc();
    std::printf("rdtsc twice: second reading larger than first: %s\n", t2 > t1 ? "yes" : "no");

    int mismatches = 0;
    for (std::uint64_t v = 1; v != 0 && v < (1ULL << 40); v = v * 3 + 1) {
        const unsigned expected = static_cast<unsigned>(std::bit_width(v)) - 1;
        if (cpu::highestBit(v) != expected) {
            ++mismatches;
        }
    }
    std::printf("bsr wrapper versus std::bit_width - 1: %d mismatches\n", mismatches);
    return mismatches == 0 ? 0 : 1;
}
