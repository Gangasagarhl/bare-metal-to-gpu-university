// bench.hpp - the SP302 measurement harness (chapter F2-51).
// Warm up, time many repetitions with a monotonic clock, report min / median / max.
#pragma once
#include <algorithm>
#include <chrono>
#include <vector>

namespace bench
{

struct Stats
{
    double minimum;  // seconds
    double median;   // seconds
    double maximum;  // seconds
};

// Tell the optimiser that 'value' is used, so the work that produced it stays.
// GNU extended asm with no instructions: it emits nothing, but the compiler must
// assume the asm reads the value (from a register or memory) and may touch memory.
// Use it on small values such as a sum, not on large objects.
template <typename T>
inline void keep(T const& value)
{
    asm volatile("" : : "r,m"(value) : "memory");
}

// Run 'work' warmups times untimed, then reps times timed; reps should be odd.
template <typename Work>
Stats run(Work&& work, int warmups = 3, int reps = 21)
{
    using clock = std::chrono::steady_clock;
    for (int i = 0; i < warmups; ++i) {
        work();
    }
    std::vector<double> seconds;
    seconds.reserve(static_cast<std::size_t>(reps));
    for (int i = 0; i < reps; ++i) {
        auto const start = clock::now();
        work();
        auto const stop = clock::now();
        seconds.push_back(std::chrono::duration<double>(stop - start).count());
    }
    std::sort(seconds.begin(), seconds.end());
    return {seconds.front(), seconds[seconds.size() / 2], seconds.back()};
}

}  // namespace bench
