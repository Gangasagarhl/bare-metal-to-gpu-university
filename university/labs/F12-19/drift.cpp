// drift.cpp - F12-19 Listing 3: counting time by adding 0.1 s, the arithmetic of the drift.
// Part 1: 0.1 has no exact binary form. Stored with k fractional bits by truncation, each
// tick loses r / (10 * 2^k) seconds, where r = 2^k mod 10. The drift after N ticks is N times
// that; the program computes it exactly with integers, then prints it as a decimal.
// Part 2: the same idea with float and double accumulation, measured by running it.
#include <cstdint>
#include <cstdio>
#include <initializer_list>

int main()
{
    std::printf("part 1: 0.1 s truncated to k fractional bits, 10 ticks per second\n");
    std::printf("%3s  %22s  %14s  %14s  %14s\n", "k", "error per tick (s)", "after 1 h (s)",
                "after 24 h (s)", "after 100 h (s)");
    for (const int k : {8, 16, 23, 32}) {
        const std::uint64_t two_k = std::uint64_t{1} << k;
        const std::uint64_t r = two_k % 10;  // 0.1 * 2^k = two_k / 10 = q + r / 10
        const long double per_tick = static_cast<long double>(r) / (10.0L * two_k);
        std::printf("%3d  %22.15Lf", k, per_tick);
        for (const long hours : {1L, 24L, 100L}) {
            const long ticks = hours * 3600 * 10;
            std::printf("  %14.6Lf", per_tick * ticks);
        }
        std::printf("\n");
    }

    std::printf("\npart 2: t += 0.1 repeated, compared with the tick count / 10\n");
    std::printf("%10s  %14s  %16s  %16s\n", "hours", "exact (s)", "float error (s)",
                "double error (s)");
    float tf = 0.0f;
    double td = 0.0;
    long ticks = 0;
    for (const long hours : {1L, 24L, 100L}) {
        while (ticks < hours * 3600 * 10) {
            tf += 0.1f;
            td += 0.1;
            ++ticks;
        }
        const double exact = ticks / 10.0;
        std::printf("%10ld  %14.1f  %16.6f  %16.9f\n", hours, exact,
                    static_cast<double>(tf) - exact, td - exact);
    }
    std::printf("\nfix: count integer ticks; convert to seconds only when printing\n");
    return 0;
}
