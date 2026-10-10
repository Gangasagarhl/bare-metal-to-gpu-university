// Recomputes every number quoted in the HW203 answer keys (university/_keys/HW203.keys.html)
// that is not read directly from a lab output file: Little's-law arithmetic, address splits,
// average memory access times, stride miss rates, the loop-nest and same-set predictions through
// the course cache model (cache.hpp), the DRAM model's address mapping, TLB reach, and the ratios
// of measured numbers quoted in the papers. Exercise values throughout (owner ruling A4).
#include <cstdint>
#include <cstdio>

#include "cache.hpp"

void littles(const char* name, double latencyNs, double gbPerSecond)
{
    const double bytes = gbPerSecond * latencyNs;            // 1 GB/s = 1 byte per ns
    std::printf("%s: %.0f ns x %.1f GB/s = %.0f bytes in flight = %.1f blocks of 64 B; "
                "one block at a time: %.3f GB/s\n", name, latencyNs, gbPerSecond, bytes, bytes / 64.0,
                64.0 / latencyNs);
}

void split(const char* name, std::uint64_t a, unsigned sets, unsigned lineBytes)
{
    unsigned b = 0;
    for (unsigned x = lineBytes; x > 1; x >>= 1) ++b;
    unsigned s = 0;
    for (unsigned x = sets; x > 1; x >>= 1) ++s;
    std::printf("%s: 0x%llX in %u sets x %u B: offset bits %u, index bits %u; offset %llu (0x%llX), "
                "set %llu (0x%llX), tag 0x%llX (%llu)\n", name, static_cast<unsigned long long>(a),
                sets, lineBytes, b, s, static_cast<unsigned long long>(a & (lineBytes - 1)),
                static_cast<unsigned long long>(a & (lineBytes - 1)),
                static_cast<unsigned long long>((a >> b) & (sets - 1)),
                static_cast<unsigned long long>((a >> b) & (sets - 1)),
                static_cast<unsigned long long>(a >> (b + s)), static_cast<unsigned long long>(a >> (b + s)));
}

std::uint64_t loopNest(unsigned sets, unsigned ways, unsigned n, bool rowsFirst)
{
    Cache cache(sets, ways, 64);
    for (unsigned a = 0; a < n; ++a) {
        for (unsigned b = 0; b < n; ++b) {
            const unsigned i = rowsFirst ? a : b;
            const unsigned j = rowsFirst ? b : a;
            cache.access(std::uint64_t{i} * n * 8 + std::uint64_t{j} * 8);
        }
    }
    return cache.misses();
}

void sameSet(unsigned lines, unsigned passes)
{
    Cache cache(64, 12, 64);
    for (unsigned p = 0; p < passes; ++p) {
        for (unsigned k = 0; k < lines; ++k) {
            cache.access(std::uint64_t{k} * 4096);
        }
    }
    std::printf("%u lines 4096 B apart, %u passes, 64 sets x 12 ways: hits %llu, misses %llu\n", lines,
                passes, static_cast<unsigned long long>(cache.hits()),
                static_cast<unsigned long long>(cache.misses()));
}

void dram(std::uint64_t a)   // the mapping of F1-39's dram_model.cpp
{
    std::printf("DRAM model 0x%llX: channel %llu, bank %llu, row %llu, column %llu\n",
                static_cast<unsigned long long>(a), static_cast<unsigned long long>((a >> 6) & 1),
                static_cast<unsigned long long>((a >> 14) & 3), static_cast<unsigned long long>(a >> 16),
                static_cast<unsigned long long>((a >> 7) & 127));
}

int main()
{
    std::printf("== Midterm ==\n");
    littles("M2 (120 ns, 16 GB/s)", 120, 16);
    std::printf("M3: 263.64 / 1.92 = %.0f x; 38.19 / 7.93 = %.1f x\n", 263.64 / 1.92, 38.19 / 7.93);
    std::printf("M4: capacity 256 x 4 x 32 = %u bytes = %u KiB\n", 256 * 4 * 32, 256 * 4 * 32 / 1024);
    split("M4", 0x2F4A7, 256, 32);
    std::printf("M6: AMAT 1.5 + 0.08 x 70 = %.1f ns; 4 %% misses: %.1f ns; hit 1.0 ns, 8 %%: %.1f ns\n",
                1.5 + 0.08 * 70, 1.5 + 0.04 * 70, 1.0 + 0.08 * 70);
    std::printf("M7: 2-byte elements, 64-byte lines: %u per line; stride 1 %.3f %%, stride 8 %.0f %%, "
                "stride 32 %.0f %%, stride 64 100 %%\n", 32, 100.0 / 32, 800.0 / 32, 3200.0 / 32);
    std::printf("M8: n = 32 doubles, 8 sets x 2 ways: rows %llu, cols %llu; 4 sets x 4 ways: cols %llu; "
                "1 set x 16 ways: rows %llu, cols %llu\n",
                static_cast<unsigned long long>(loopNest(8, 2, 32, true)),
                static_cast<unsigned long long>(loopNest(8, 2, 32, false)),
                static_cast<unsigned long long>(loopNest(4, 4, 32, false)),
                static_cast<unsigned long long>(loopNest(1, 16, 32, true)),
                static_cast<unsigned long long>(loopNest(1, 16, 32, false)));
    std::printf("M9: 536800 / 4714810 = %.2f %%; 4194304 / 8 = %u lines\n", 100.0 * 536800 / 4714810,
                4194304 / 8);
    std::printf("M11: fill / read = 7.43 / 12.33 = %.0f %%\n", 100.0 * 7.43 / 12.33);
    std::printf("M12: 228.48 / 38.19 = %.1f x; 228.48 / 1.92 = %.0f x\n", 228.48 / 38.19, 228.48 / 1.92);
    std::printf("M13: stride 1920 x 4 = %u bytes = %.2f pages of 4096; row order stride 4 B: miss rate 1/16 = %.2f %%\n",
                1920 * 4, 1920 * 4 / 4096.0, 100.0 / 16);
    std::printf("== Final ==\n");
    littles("F2 (90 ns, 12 GB/s)", 90, 12);
    littles("F2 (45 ns, 12 GB/s)", 45, 12);
    split("F3", 0x1C3B8, 64, 64);
    split("F3 +4096", 0x1C3B8 + 4096, 64, 64);
    split("F3 +8192", 0x1C3B8 + 8192, 64, 64);
    sameSet(16, 3);
    sameSet(12, 3);
    sameSet(13, 3);
    std::printf("F5: 4-byte ints, 128-byte lines: 32 per line; stride 1 %.3f %%, stride 16 %.0f %%, stride 32 100 %%; "
                "stride table 6.26 / 1.42 = %.1f x, 1.42 / 0.46 = %.1f x\n", 100.0 / 32, 1600.0 / 32,
                6.26 / 1.42, 1.42 / 0.46);
    std::printf("F6: n = 24 doubles, 8 sets x 2 ways: rows %llu, cols %llu; 1 set x 16 ways: cols %llu; "
                "n = 8: rows %llu, cols %llu\n",
                static_cast<unsigned long long>(loopNest(8, 2, 24, true)),
                static_cast<unsigned long long>(loopNest(8, 2, 24, false)),
                static_cast<unsigned long long>(loopNest(1, 16, 24, false)),
                static_cast<unsigned long long>(loopNest(8, 2, 8, true)),
                static_cast<unsigned long long>(loopNest(8, 2, 8, false)));
    std::printf("F7: write-back 7 reads + 6 write-backs = %d transactions, %d bytes (16-byte lines); "
                "write-through 6 reads + 6 single writes = %d transactions, %d bytes (8-byte words)\n",
                7 + 6, (7 + 6) * 16, 6 + 6, 6 * 16 + 6 * 8);
    std::printf("F11: 2489.6 / 283.7 = %.2f x; 1109.4 / 121.5 = %.2f x; 2489.6 / 125.8 = %.1f x; "
                "struct of two 4-byte atomics = 8 bytes, %d per 64-byte line, 16 workers = %d lines\n",
                2489.6 / 283.7, 1109.4 / 121.5, 2489.6 / 125.8, 64 / 8, 16 * 8 / 64);
    std::printf("F13: 724 / 200000 = %.3f %%; 916 / 200000 = %.3f %%\n", 100.0 * 724 / 200000,
                100.0 * 916 / 200000);
    std::printf("F15: 0x5A3C7F10: offset 0x%llX, level-1 %llu, level-2 %llu, level-3 %llu, level-4 %llu\n",
                0x5A3C7F10ull & 0xFFF, (0x5A3C7F10ull >> 12) & 0x1FF, (0x5A3C7F10ull >> 21) & 0x1FF,
                (0x5A3C7F10ull >> 30) & 0x1FF, (0x5A3C7F10ull >> 39) & 0x1FF);
    std::printf("F16: 64 entries x 4 KiB = %u KiB; 64 x 2 MiB = %u MiB; 16384 blocks: 91.11 / 11.22 = %.1f x, "
                "91.11 / 46.58 = %.2f x; 16384 x 64 B = %u KiB of lines over %u MiB of 4 KiB pages\n",
                64 * 4, 64 * 2, 91.11 / 11.22, 91.11 / 46.58, 16384 * 64 / 1024, 16384 * 4 / 1024);
    dram(0x2C0C0);
    for (unsigned i = 0; i < 4; ++i) dram(std::uint64_t{i} * 0x8000);
    for (unsigned i = 0; i < 4; ++i) dram(std::uint64_t{i} * 0x8040);
    littles("F19 (263.6 ns, 7.53 GB/s)", 263.6, 7.53);
    std::printf("F19: 23.17 / 7.53 = %.2f x\n", 23.17 / 7.53);
    std::printf("F20: 4096 / 512 = %d; 256 KiB / 48 KiB = %.2f; 6.26 / 1.42 = %.1f x; 16.71 / 15.68 = %.2f x\n",
                4096 / 512, 256.0 / 48, 6.26 / 1.42, 16.71 / 15.68);
    std::printf("== Practical ==\n");
    std::printf("P: n = 8 doubles: 8 a-lines (sets 0-7) + 8 b-lines (sets 0-7) = 16 lines; 8 sets x 2 ways = %d slots; "
                "n = 512: row = %u bytes = sets x line, so a column's lines share one set: %u lines in 12 ways\n",
                8 * 2, 512 * 8, 512);
    std::printf("P: ratios of the measured times are computed by run.sh from transpose_time.out (transpose_time_ratios.out)\n");
    return 0;
}
