// Measurement (F2-38): what one increment costs: plain, atomic alone, atomic shared by four
// threads, and four private atomics that are neighbours (false sharing) or 64 bytes apart.
// Built with -O2, no sanitizers, by run.sh.
#include <algorithm>
#include <atomic>
#include <chrono>
#include <iostream>
#include <new>
#include <thread>
#include <vector>

using Clock = std::chrono::steady_clock;
constexpr long kPerThread = 5'000'000;

struct Neighbours
{
    std::atomic<long> c[4];  // four counters in 32 consecutive bytes
};

struct Padded
{
    alignas(64) std::atomic<long> c;  // each counter starts its own 64-byte block
};

template <typename Body>
double nsPerOp(int threads, Body body)
{
    std::vector<double> runs;
    for (int r = 0; r < 5; ++r) {
        const auto t0 = Clock::now();
        std::vector<std::thread> ts;
        for (int t = 0; t < threads; ++t) {
            ts.emplace_back(body, t);
        }
        for (auto& t : ts) {
            t.join();
        }
        const auto t1 = Clock::now();
        runs.push_back(std::chrono::duration<double, std::nano>(t1 - t0).count() / kPerThread);  // per thread
    }
    std::sort(runs.begin(), runs.end());
    return runs[2];
}

void show(const char* what, double ns)
{
    std::cout << what << static_cast<long>(ns * 100) / 100.0 << " ns per increment, as seen by each thread\n";
}

int main()
{
    std::cout << "std::hardware_destructive_interference_size = "
              << std::hardware_destructive_interference_size << " bytes\n";
    long plainResult = 0;
    show("plain long, 1 thread:                 ", nsPerOp(1, [&plainResult](int) {
        long x = 0;
        for (long i = 0; i < kPerThread; ++i) {
            ++x;
            asm volatile("" : "+r"(x));  // keep every increment
        }
        plainResult = x;
    }));
    std::atomic<long> one{0};
    show("atomic fetch_add, 1 thread:           ", nsPerOp(1, [&one](int) {
        for (long i = 0; i < kPerThread; ++i) {
            one.fetch_add(1);
        }
    }));
    std::atomic<long> shared{0};
    show("atomic fetch_add, 4 threads, 1 counter:", nsPerOp(4, [&shared](int) {
        for (long i = 0; i < kPerThread; ++i) {
            shared.fetch_add(1);
        }
    }));
    Neighbours nb{};
    show("2 threads, own counters, neighbours:   ", nsPerOp(2, [&nb](int t) {
        for (long i = 0; i < kPerThread; ++i) {
            nb.c[t].fetch_add(1);
        }
    }));
    std::vector<Padded> pad(4);
    show("2 threads, own counters, 64 B apart:   ", nsPerOp(2, [&pad](int t) {
        for (long i = 0; i < kPerThread; ++i) {
            pad[t].c.fetch_add(1);
        }
    }));
    show("4 threads, own counters, neighbours:   ", nsPerOp(4, [&nb](int t) {
        for (long i = 0; i < kPerThread; ++i) {
            nb.c[t].fetch_add(1);
        }
    }));
    show("4 threads, own counters, 64 B apart:   ", nsPerOp(4, [&pad](int t) {
        for (long i = 0; i < kPerThread; ++i) {
            pad[t].c.fetch_add(1);
        }
    }));
    std::cout << "(check: plain " << plainResult << "; shared counter after 5 runs of 4 x " << kPerThread
              << " = " << shared.load() << ")\n";
    return 0;
}
