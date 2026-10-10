// branches.cc - sum the bytes >= 128 in an array: sorted versus shuffled data,
// with a branch and without one. Usage: ./branches [n] [reps] [both|sorted|shuffled]
#include "bench.hpp"
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <random>
#include <string>
#include <vector>

// A conditional jump guards the addition.
[[gnu::noinline]] long sumBranchy(std::vector<std::uint8_t> const& v)
{
    long s = 0;
    for (std::uint8_t b : v) {
        if (b >= 128) {
            s += b;
        }
    }
    return s;
}

// No jump depends on the data: a mask (all ones or all zeros) selects the value.
[[gnu::noinline]] long sumBranchless(std::vector<std::uint8_t> const& v)
{
    long s = 0;
    for (std::uint8_t b : v) {
        long const mask = -static_cast<long>(b >= 128);  // 0 or -1 (all bits set)
        s += mask & b;
    }
    return s;
}

int main(int argc, char** argv)
{
    std::size_t const n = argc > 1 ? std::strtoul(argv[1], nullptr, 10) : (std::size_t{1} << 24);
    int const reps = argc > 2 ? std::atoi(argv[2]) : 21;
    std::string const pick = argc > 3 ? argv[3] : "both";
    std::vector<std::uint8_t> shuffled(n);
    std::mt19937 rng(7);
    std::uniform_int_distribution<int> byte(0, 255);
    for (auto& b : shuffled) {
        b = static_cast<std::uint8_t>(byte(rng));
    }
    std::vector<std::uint8_t> sorted = shuffled;
    std::sort(sorted.begin(), sorted.end());
    std::printf("n = %zu bytes, median of %d runs\n", n, reps);
    std::printf("%-14s %-9s %10s %10s\n", "function", "data", "ns/elem", "result");
    for (int which = 0; which < 2; ++which) {
        auto const& data = which == 0 ? sorted : shuffled;
        char const* name = which == 0 ? "sorted" : "shuffled";
        if (pick != "both" && pick != name) {
            continue;
        }
        long r1 = 0;
        long r2 = 0;
        auto const a = bench::run([&] { r1 = sumBranchy(data); bench::keep(r1); }, reps > 1 ? 2 : 0, reps);
        auto const b = bench::run([&] { r2 = sumBranchless(data); bench::keep(r2); }, reps > 1 ? 2 : 0, reps);
        double const e = static_cast<double>(n);
        std::printf("%-14s %-9s %10.3f %10ld\n", "sumBranchy", name, a.median * 1e9 / e, r1);
        std::printf("%-14s %-9s %10.3f %10ld\n", "sumBranchless", name, b.median * 1e9 / e, r2);
    }
    return 0;
}
