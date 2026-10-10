// F6-14 forensic evidence: replays the index arithmetic of sumBytes32 and sumBytes64 for every
// block, for several input sizes n (every byte = 1, so the right answer is n). No data is stored:
// for each block we count which of its 2 x 256 loads the kernel performs.
// The 32-bit arithmetic follows the C++ rules for the types in overflow.cu:
//   blockIdx.x * (blockDim.x * 2) + threadIdx.x  is unsigned 32-bit (wraps modulo 2^32),
//   storing it in int gives a negative value from 2^31 on (C++20: conversion is modulo 2^32),
//   "i < n" converts the negative int to a huge size_t: false, the load is skipped;
//   "i + blockDim.x" is unsigned 32-bit again, so that second load still finds the right byte.
#include <cstdint>
#include <cstdio>

constexpr std::uint32_t BLOCK = 256;

std::uint64_t emulate(std::uint64_t n, bool index64, std::uint64_t& firstBadBlock)
{
    const std::uint64_t blocks = (n + 2 * BLOCK - 1) / (2 * BLOCK);
    std::uint64_t total = 0;
    firstBadBlock = 0;
    bool seenBad = false;
    for (std::uint64_t b = 0; b < blocks; ++b) {
        std::uint64_t loads = 0;
        // first load of every thread t: index base + t
        std::uint64_t base64 = b * 2 * BLOCK;
        std::int32_t base32 = static_cast<std::int32_t>(static_cast<std::uint32_t>(base64));
        for (int half = 0; half < 2; ++half) {
            std::uint64_t start = base64 + half * BLOCK;              // true index of thread 0
            if (index64 || half == 1 || base32 >= 0) {
                // 64-bit index, or the unsigned second load, or no wrap yet: loads index start + t
                std::uint64_t valid = (start >= n) ? 0 : (n - start < BLOCK ? n - start : BLOCK);
                loads += valid;
            }
            // else: the first load of every thread in this block is skipped (i is negative)
        }
        std::uint64_t should = (base64 >= n) ? 0 : (n - base64 < 2 * BLOCK ? n - base64 : 2 * BLOCK);
        if (loads != should && !seenBad) { seenBad = true; firstBadBlock = b; }
        total += loads;
    }
    return total;
}

int main()
{
    const std::uint64_t sizes[] = {1ull << 20, 1ull << 28, (1ull << 31) - 512, 1ull << 31,
                                   (1ull << 31) + 512, 3ull << 30};
    std::printf("%-14s %-14s %-14s %-12s %s\n", "n (bytes)", "sumBytes32", "sumBytes64", "missing",
                "first wrong block");
    for (std::uint64_t n : sizes) {
        std::uint64_t bad32 = 0, bad64 = 0;
        std::uint64_t s32 = emulate(n, false, bad32);
        std::uint64_t s64 = emulate(n, true, bad64);
        std::printf("%-14llu %-14llu %-14llu %-12llu ", static_cast<unsigned long long>(n),
                    static_cast<unsigned long long>(s32), static_cast<unsigned long long>(s64),
                    static_cast<unsigned long long>(n - s32));
        if (s32 != n) { std::printf("%llu\n", static_cast<unsigned long long>(bad32)); }
        else { std::printf("-\n"); }
    }
    std::printf("2^31 = %llu; 2^31 / (2 x %u) = %llu\n", 1ull << 31, BLOCK, (1ull << 31) / (2 * BLOCK));
    return 0;
}
