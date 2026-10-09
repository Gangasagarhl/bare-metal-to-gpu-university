// Measurement (F2-34): the same prime count with 1, 2, 4 and 8 threads; also the cost of
// starting and joining one thread. Built with -O2 and no sanitizer by run.sh.
#include <algorithm>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <thread>
#include <vector>

bool isPrime(std::uint32_t x)
{
    if (x < 2) {
        return false;
    }
    for (std::uint32_t d = 2; d * d <= x; ++d) {
        if (x % d == 0) {
            return false;
        }
    }
    return true;
}

// Thread t tests t+2, t+2+T, t+2+2T, ... so that every thread gets small and large numbers.
long countPrimes(std::uint32_t limit, unsigned numThreads)
{
    std::vector<long> found(numThreads, 0);
    std::vector<std::thread> workers;
    for (unsigned t = 0; t < numThreads; ++t) {
        workers.emplace_back([&found, t, numThreads, limit] {
            long local = 0;
            for (std::uint32_t x = 2 + t; x < limit; x += numThreads) {
                local += isPrime(x) ? 1 : 0;
            }
            found[t] = local;
        });
    }
    for (std::thread& w : workers) {
        w.join();
    }
    long total = 0;
    for (long f : found) {
        total += f;
    }
    return total;
}

double medianMs(std::vector<double> v)
{
    std::sort(v.begin(), v.end());
    return v[v.size() / 2];
}

int main()
{
    using Clock = std::chrono::steady_clock;
    const std::uint32_t limit = 3'000'000;
    std::cout << "hardware_concurrency() = " << std::thread::hardware_concurrency() << '\n';
    double base = 0;
    for (unsigned threads : {1u, 2u, 4u, 8u}) {
        std::vector<double> times;
        long primes = 0;
        for (int run = 0; run < 5; ++run) {
            const auto t0 = Clock::now();
            primes = countPrimes(limit, threads);
            const auto t1 = Clock::now();
            times.push_back(std::chrono::duration<double, std::milli>(t1 - t0).count());
        }
        const double ms = medianMs(times);
        if (threads == 1) {
            base = ms;
        }
        std::cout << "threads " << threads << ": primes below " << limit << " = " << primes
                  << ", median of 5 runs " << static_cast<long>(ms) << " ms, speedup "
                  << static_cast<double>(static_cast<long>(base / ms * 100)) / 100 << '\n';
    }
    const int starts = 2000;
    const auto t0 = Clock::now();
    for (int i = 0; i < starts; ++i) {
        std::thread t([] {});
        t.join();
    }
    const auto t1 = Clock::now();
    const double us = std::chrono::duration<double, std::micro>(t1 - t0).count() / starts;
    std::cout << "start + join of an empty thread, mean of " << starts << ": "
              << static_cast<long>(us) << " microseconds\n";
    return 0;
}
