// False sharing, measured: each thread atomically adds 1 to ITS OWN counter many times.
// No data is shared, yet when the counters sit in the same 64-byte block the threads
// slow each other down.
// Times are measurements on the machine that ran this program, not specifications.
#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <thread>
#include <vector>

struct Packed
{
    std::atomic<std::uint64_t> count{0};     // 8 bytes: neighbours share one block
};

struct alignas(64) Padded
{
    std::atomic<std::uint64_t> count{0};     // each counter starts its own 64-byte block
};

template <typename Counter>
double runMs(int threads, std::uint64_t perThread)
{
    std::vector<Counter> counters(threads);
    const auto t0 = std::chrono::steady_clock::now();
    std::vector<std::thread> pool;
    for (int t = 0; t < threads; ++t) {
        pool.emplace_back([&counters, t, perThread] {
            for (std::uint64_t i = 0; i < perThread; ++i) {
                counters[t].count.fetch_add(1, std::memory_order_relaxed);  // one atomic add
            }
        });
    }
    for (std::thread& th : pool) {
        th.join();
    }
    const auto t1 = std::chrono::steady_clock::now();
    for (const Counter& c : counters) {
        if (c.count != perThread) {
            std::printf("unexpected count\n");
        }
    }
    return std::chrono::duration<double, std::milli>(t1 - t0).count();
}

int main(int argc, char** argv)
{
    const std::uint64_t perThread = argc > 1 ? std::stoull(argv[1]) : 20'000'000;
    std::printf("sizeof(Packed) = %zu bytes, sizeof(Padded) = %zu bytes\n", sizeof(Packed),
                sizeof(Padded));
    std::printf("%u hardware threads reported; %llu increments per thread\n",
                std::thread::hardware_concurrency(), static_cast<unsigned long long>(perThread));
    std::printf("%8s %14s %14s\n", "threads", "packed (ms)", "padded (ms)");
    for (const int threads : {1, 2, 4}) {
        std::printf("%8d %14.1f %14.1f\n", threads, runMs<Packed>(threads, perThread),
                    runMs<Padded>(threads, perThread));
    }
    return 0;
}
