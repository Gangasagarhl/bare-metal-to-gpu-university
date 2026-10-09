// traverse.cc - the same sum over an n x n matrix, row by row and column by column.
// Usage: ./traverse [n] [reps]   (defaults 4096 21; the cachegrind step uses small values)
#include "bench.hpp"
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <vector>

// Row-major storage: element (r, c) lives at index r * n + c.
[[gnu::noinline]] float sumRows(std::vector<float> const& m, std::size_t n)
{
    float s = 0.0f;
    for (std::size_t r = 0; r < n; ++r) {
        for (std::size_t c = 0; c < n; ++c) {
            s += m[r * n + c];  // neighbours in memory: one cache line serves 16 floats
        }
    }
    return s;
}

[[gnu::noinline]] float sumColumns(std::vector<float> const& m, std::size_t n)
{
    float s = 0.0f;
    for (std::size_t c = 0; c < n; ++c) {
        for (std::size_t r = 0; r < n; ++r) {
            s += m[r * n + c];  // jumps n * 4 bytes every step
        }
    }
    return s;
}

int main(int argc, char** argv)
{
    std::size_t const n = argc > 1 ? std::strtoul(argv[1], nullptr, 10) : 4096;
    int const reps = argc > 2 ? std::atoi(argv[2]) : 21;
    std::vector<float> m(n * n, 1.0f);
    auto const rows = bench::run([&] { bench::keep(sumRows(m, n)); }, reps > 1 ? 2 : 0, reps);
    auto const cols = bench::run([&] { bench::keep(sumColumns(m, n)); }, reps > 1 ? 2 : 0, reps);
    double const elems = static_cast<double>(n) * static_cast<double>(n);
    std::printf("n = %zu (matrix of %.1f MiB), median of %d runs\n", n, elems * 4 / (1 << 20), reps);
    std::printf("sumRows:    %9.3f ms  %6.3f ns per element\n", rows.median * 1e3, rows.median * 1e9 / elems);
    std::printf("sumColumns: %9.3f ms  %6.3f ns per element\n", cols.median * 1e3, cols.median * 1e9 / elems);
    std::printf("columns / rows = %.1f\n", cols.median / rows.median);
    return 0;
}
