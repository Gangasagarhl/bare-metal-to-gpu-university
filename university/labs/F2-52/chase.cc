// chase.cc - average time of one dependent load as the working set grows.
// A random cyclic permutation (Sattolo's algorithm) defeats the hardware prefetcher, and
// each load's address comes from the previous load, so loads cannot overlap.
#include "bench.hpp"
#include <cstddef>
#include <cstdio>
#include <numeric>
#include <random>
#include <utility>
#include <vector>

[[gnu::noinline]] std::size_t chase(std::vector<std::size_t> const& next, std::size_t steps)
{
    std::size_t i = 0;
    for (std::size_t k = 0; k < steps; ++k) {
        i = next[i];
    }
    return i;
}

int main()
{
    std::mt19937_64 rng(42);
    std::size_t const steps = std::size_t{1} << 22;
    std::printf("%12s %14s\n", "working set", "ns per load");
    for (std::size_t bytes = 4096; bytes <= (std::size_t{1} << 30); bytes *= 2) {
        std::size_t const n = bytes / sizeof(std::size_t);
        std::vector<std::size_t> next(n);
        std::iota(next.begin(), next.end(), std::size_t{0});
        for (std::size_t i = n - 1; i > 0; --i) {  // Sattolo: one cycle through all slots
            std::uniform_int_distribution<std::size_t> pick(0, i - 1);
            std::swap(next[i], next[pick(rng)]);
        }
        auto const s = bench::run([&] { bench::keep(chase(next, steps)); }, 1, 7);
        double const ns = s.median * 1e9 / static_cast<double>(steps);
        if (bytes < (1 << 20)) {
            std::printf("%9zu KiB %14.2f\n", bytes >> 10, ns);
        } else {
            std::printf("%9zu MiB %14.2f\n", bytes >> 20, ns);
        }
    }
    return 0;
}
