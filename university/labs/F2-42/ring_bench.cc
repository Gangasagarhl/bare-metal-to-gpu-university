// Measurement (F2-42): one producer and one consumer moving 5 000 000 integers through
// (a) the lock-free SpscRing and (b) F2-37's BoundedQueue (mutex + condition variables),
// both with 1024 slots. Built with -O2, no sanitizers, by run.sh.
#include <algorithm>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <thread>
#include <vector>

#include "../F2-37/bounded_queue.hpp"
#include "spsc_ring.hpp"

using Clock = std::chrono::steady_clock;
constexpr std::uint32_t kItems = 5'000'000;

double ringOnce()
{
    SpscRing<std::uint32_t> ring(1024);
    std::uint64_t sum = 0;
    const auto t0 = Clock::now();
    std::thread p([&] {
        for (std::uint32_t v = 0; v < kItems; ++v) {
            while (!ring.tryPush(v)) {
            }
        }
    });
    std::thread c([&] {
        for (std::uint32_t got = 0; got < kItems;) {
            if (auto v = ring.tryPop()) {
                sum += *v;
                ++got;
            }
        }
    });
    p.join();
    c.join();
    const double s = std::chrono::duration<double>(Clock::now() - t0).count();
    return sum == static_cast<std::uint64_t>(kItems) * (kItems - 1) / 2 ? kItems / s : -1;
}

double queueOnce()
{
    BoundedQueue<std::uint32_t> q(1024);
    std::uint64_t sum = 0;
    const auto t0 = Clock::now();
    std::thread p([&] {
        for (std::uint32_t v = 0; v < kItems; ++v) {
            q.push(v);
        }
        q.close();
    });
    std::thread c([&] {
        while (auto v = q.pop()) {
            sum += *v;
        }
    });
    p.join();
    c.join();
    const double s = std::chrono::duration<double>(Clock::now() - t0).count();
    return sum == static_cast<std::uint64_t>(kItems) * (kItems - 1) / 2 ? kItems / s : -1;
}

template <typename F>
double median(F f)
{
    std::vector<double> v;
    for (int r = 0; r < 5; ++r) {
        v.push_back(f());
    }
    std::sort(v.begin(), v.end());
    return v[2];
}

int main()
{
    std::cout << "1 producer, 1 consumer, " << kItems << " items, 1024 slots, median of 5 runs\n";
    std::cout << "SpscRing (lock-free):          " << static_cast<long>(median(ringOnce) / 1e6)
              << " million items/s\n";
    std::cout << "BoundedQueue (mutex + condvar): " << static_cast<long>(median(queueOnce) / 1e6)
              << " million items/s\n";
    return 0;
}
