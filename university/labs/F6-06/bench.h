// F6-06 Listing 1: the core of the CU201 benchmark harness (the course project).
// It knows nothing about GPUs: a back end hands it a function that runs the work
// once and returns the elapsed time in milliseconds (CPU clock, CUDA events or
// HIP events). The harness does the warm-up, repeats, and summarises.
#pragma once
#include <algorithm>
#include <cstdio>
#include <string>
#include <vector>

namespace bench {

struct Stats
{
    int runs = 0;
    double minMs = 0.0;
    double medianMs = 0.0;
    double meanMs = 0.0;
    double maxMs = 0.0;
};

// Median: middle value of the sorted list (mean of the two middle values if even).
inline Stats summarize(std::vector<double> ms)
{
    Stats s;
    if (ms.empty()) { return s; }
    std::sort(ms.begin(), ms.end());
    const std::size_t n = ms.size();
    s.runs = static_cast<int>(n);
    s.minMs = ms.front();
    s.maxMs = ms.back();
    s.medianMs = (n % 2 == 1) ? ms[n / 2] : 0.5 * (ms[n / 2 - 1] + ms[n / 2]);
    double sum = 0.0;
    for (double v : ms) { sum += v; }
    s.meanMs = sum / static_cast<double>(n);
    return s;
}

// Runs timeOnce() `warmup` times without keeping the result, then `runs` times.
template <class F>
std::vector<double> measure(F timeOnce, int warmup, int runs)
{
    for (int i = 0; i < warmup; ++i) { (void)timeOnce(); }
    std::vector<double> ms;
    ms.reserve(static_cast<std::size_t>(runs));
    for (int i = 0; i < runs; ++i) { ms.push_back(timeOnce()); }
    return ms;
}

// Effective bandwidth in GB/s (10^9 bytes per second) from bytes moved and milliseconds.
inline double gbPerS(double bytes, double ms)
{
    return ms > 0.0 ? bytes / (ms * 1.0e-3) / 1.0e9 : 0.0;
}

struct Row
{
    std::string backend;
    std::string kernel;
    long long n = 0;
    double bytes = 0.0;   // bytes read + bytes written by one run
    Stats stats;
};

inline void printTable(const std::vector<Row>& rows)
{
    std::printf("%-6s %-14s %12s %5s %11s %11s %11s %9s\n", "back", "kernel", "n", "runs",
                "min ms", "median ms", "max ms", "GB/s");
    for (const Row& r : rows) {
        std::printf("%-6s %-14s %12lld %5d %11.4f %11.4f %11.4f %9.2f\n", r.backend.c_str(),
                    r.kernel.c_str(), r.n, r.stats.runs, r.stats.minMs, r.stats.medianMs,
                    r.stats.maxMs, gbPerS(r.bytes, r.stats.medianMs));
    }
}

}  // namespace bench
