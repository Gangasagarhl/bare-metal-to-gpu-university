// noise.cc - the same work timed 21 times, alone or next to busy "neighbour" threads.
// Usage: ./noise [number of busy neighbour threads]
#include "bench.hpp"
#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <thread>
#include <vector>

[[gnu::noinline]] double work(std::vector<float> const& v)
{
    float s = 0.0f;
    for (float x : v) {
        s += x;
    }
    return s;
}

int main(int argc, char** argv)
{
    int const neighbours = argc > 1 ? std::atoi(argv[1]) : 0;
    std::atomic<bool> stop{false};
    std::vector<std::thread> busy;
    for (int i = 0; i < neighbours; ++i) {
        busy.emplace_back([&stop] {
            unsigned long long x = 1;
            while (!stop.load(std::memory_order_relaxed)) {
                x = x * 6364136223846793005ULL + 1;
                bench::keep(x);
            }
        });
    }
    std::vector<float> v(1 << 22, 1.0f);  // 16 MiB of floats
    std::vector<double> ms;
    for (int r = 0; r < 21; ++r) {
        auto const s = bench::run([&] { bench::keep(work(v)); }, 0, 1);
        ms.push_back(s.median * 1e3);
    }
    stop = true;
    for (auto& t : busy) {
        t.join();
    }
    std::printf("busy neighbour threads: %d (machine has %u CPUs)\n", neighbours,
                std::thread::hardware_concurrency());
    std::printf("run times in ms, in the order measured:\n");
    for (std::size_t i = 0; i < ms.size(); ++i) {
        std::printf("%7.3f%s", ms[i], (i % 7 == 6) ? "\n" : " ");
    }
    std::vector<double> sorted = ms;
    std::sort(sorted.begin(), sorted.end());
    double const med = sorted[sorted.size() / 2];
    std::printf("min %.3f  median %.3f  max %.3f  ms;  (max-min)/median = %.1f %%\n",
                sorted.front(), med, sorted.back(), 100.0 * (sorted.back() - sorted.front()) / med);
    return 0;
}
