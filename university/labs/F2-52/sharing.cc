// sharing.cc - forensic evidence: four threads, four private counters, two layouts.
#include "bench.hpp"
#include <cstddef>
#include <cstdio>
#include <thread>
#include <vector>

struct Packed
{
    long count[4];  // the four counters are neighbours in memory
};

struct alignas(64) Padded64
{
    long count;  // one counter per 64-byte block
};

template <typename CountOf>
double runThreads(CountOf countOf, long iterations)
{
    auto const s = bench::run([&] {
        std::vector<std::thread> t;
        for (int k = 0; k < 4; ++k) {
            t.emplace_back([&countOf, k, iterations] {
                long& c = countOf(k);
                for (long i = 0; i < iterations; ++i) {
                    c = c + 1;
                    bench::keep(c);  // force a real load and store every iteration
                }
            });
        }
        for (auto& th : t) {
            th.join();
        }
    }, 1, 11);
    return s.median;
}

int main()
{
    long const iterations = 20'000'000;
    Packed packed{};
    std::vector<Padded64> padded(4);
    std::printf("layout A (Packed): counter addresses relative to the first:");
    for (int k = 0; k < 4; ++k) {
        std::printf(" +%td", reinterpret_cast<char*>(&packed.count[k]) - reinterpret_cast<char*>(&packed.count[0]));
    }
    std::printf("\nlayout B (Padded64): counter addresses relative to the first:");
    for (int k = 0; k < 4; ++k) {
        std::printf(" +%td", reinterpret_cast<char*>(&padded[k].count) - reinterpret_cast<char*>(&padded[0].count));
    }
    double const a = runThreads([&](int k) -> long& { return packed.count[k]; }, iterations);
    double const b = runThreads([&](int k) -> long& { return padded[k].count; }, iterations);
    std::printf("\n4 threads x %ld increments each, median of 11 runs\n", iterations);
    std::printf("layout A: %8.1f ms\nlayout B: %8.1f ms\nA / B = %.1f\n", a * 1e3, b * 1e3, a / b);
    return 0;
}
