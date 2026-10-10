// F6-06 Listing 2: the harness with its CPU back end (std::chrono::steady_clock).
// It measures a CPU SAXPY, prints every run, and summarises. The numbers are this
// build container's CPU, not a GPU.
#include <chrono>
#include <cstdio>
#include <vector>
#include "bench.h"

static void saxpyCpu(float a, const std::vector<float>& x, std::vector<float>& y)
{
    for (std::size_t i = 0; i < x.size(); ++i) { y[i] = a * x[i] + y[i]; }
}

int main()
{
    // Part A: why the median? A fixed list with one disturbed run.
    const std::vector<double> sample = {2.0, 2.1, 1.9, 2.0, 9.0, 2.2, 2.0};
    const bench::Stats s = bench::summarize(sample);
    std::printf("A. sample of 7 runs: min %.2f  median %.2f  mean %.2f  max %.2f\n", s.minMs,
                s.medianMs, s.meanMs, s.maxMs);

    // Part B: a real measurement on this machine's CPU.
    const std::size_t n = 1 << 22;
    std::vector<float> x(n, 1.0f), y(n, 2.0f);
    auto timeOnce = [&]() {
        const auto t0 = std::chrono::steady_clock::now();
        saxpyCpu(0.5f, x, y);
        const auto t1 = std::chrono::steady_clock::now();
        return std::chrono::duration<double, std::milli>(t1 - t0).count();
    };
    const std::vector<double> cold = bench::measure(timeOnce, 0, 5);   // no warm-up
    std::printf("B. first five runs without warm-up (ms):");
    for (double v : cold) { std::printf(" %.3f", v); }
    std::printf("\n");

    const std::vector<double> runs = bench::measure(timeOnce, 3, 21);  // protocol: 3 + 21
    bench::Row row{"cpu", "saxpy", static_cast<long long>(n), 3.0 * 4.0 * n, bench::summarize(runs)};
    std::printf("C. harness table (3 warm-up runs, then 21 timed runs):\n");
    bench::printTable({row});
    std::printf("y[0] = %.1f (keeps the work from being optimised away)\n", y[0]);
    return 0;
}
