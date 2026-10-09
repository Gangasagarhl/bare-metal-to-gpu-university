// dce.cc - the benchmark that measures nothing. Built twice: -O0 and -O2 (see run.sh).
#include "bench.hpp"
#include <cstdio>
#include <vector>

// The result is thrown away: an optimiser may delete the whole loop.
[[gnu::noinline]] void sumDiscarded(std::vector<double> const& v)
{
    double s = 0.0;
    for (double x : v) {
        s += x * x;
    }
    (void)s;
}

// The same loop, but the result is "used" through bench::keep.
[[gnu::noinline]] void sumKept(std::vector<double> const& v)
{
    double s = 0.0;
    for (double x : v) {
        s += x * x;
    }
    bench::keep(s);
}

int main()
{
    std::vector<double> v(1 << 20, 1.5);
    auto const a = bench::run([&] { sumDiscarded(v); });
    auto const b = bench::run([&] { sumKept(v); });
    double const n = static_cast<double>(v.size());
    std::printf("elements per call: %zu, 3 warm-ups, 21 timed runs each\n", v.size());
    std::printf("sumDiscarded: median %10.3f us  (%.3f ns per element)\n", a.median * 1e6, a.median * 1e9 / n);
    std::printf("sumKept:      median %10.3f us  (%.3f ns per element)\n", b.median * 1e6, b.median * 1e9 / n);
    return 0;
}
