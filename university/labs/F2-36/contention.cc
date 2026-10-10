// Measurement (F2-36): the cost of one shared mutex as threads are added, compared with
// counting privately and merging once. Built with -O2, no sanitizers, by run.sh.
#include <algorithm>
#include <chrono>
#include <iostream>
#include <mutex>
#include <thread>
#include <vector>

using Clock = std::chrono::steady_clock;

// Every increment takes the shared lock.
double sharedLockNs(int threads, long perThread, long& result)
{
    std::mutex m;
    long counter = 0;
    const auto t0 = Clock::now();
    std::vector<std::thread> ts;
    for (int t = 0; t < threads; ++t) {
        ts.emplace_back([&] {
            for (long i = 0; i < perThread; ++i) {
                std::lock_guard<std::mutex> g(m);
                ++counter;
            }
        });
    }
    for (auto& t : ts) {
        t.join();
    }
    const auto t1 = Clock::now();
    result = counter;
    return std::chrono::duration<double, std::nano>(t1 - t0).count() / (threads * perThread);
}

// Each thread counts in a local variable and takes the lock once at the end.
double localThenMergeNs(int threads, long perThread, long& result)
{
    std::mutex m;
    long counter = 0;
    const auto t0 = Clock::now();
    std::vector<std::thread> ts;
    for (int t = 0; t < threads; ++t) {
        ts.emplace_back([&] {
            long local = 0;
            for (long i = 0; i < perThread; ++i) {
                ++local;
                asm volatile("" : "+r"(local));  // keep the loop: stop the compiler folding it
            }
            std::lock_guard<std::mutex> g(m);
            counter += local;
        });
    }
    for (auto& t : ts) {
        t.join();
    }
    const auto t1 = Clock::now();
    result = counter;
    return std::chrono::duration<double, std::nano>(t1 - t0).count() / (threads * perThread);
}

template <typename F>
double median(F f, int threads, long perThread, long& result)
{
    std::vector<double> v;
    for (int r = 0; r < 5; ++r) {
        v.push_back(f(threads, perThread, result));
    }
    std::sort(v.begin(), v.end());
    return v[2];
}

int main()
{
    const long perThread = 2'000'000;
    std::cout << "ns per increment (median of 5 runs), " << perThread << " increments per thread\n";
    for (int threads : {1, 2, 4}) {
        long a = 0;
        long b = 0;
        const double shared = median(sharedLockNs, threads, perThread, a);
        const double local = median(localThenMergeNs, threads, perThread, b);
        std::cout << "threads " << threads << ": shared lock " << static_cast<long>(shared * 10) / 10.0
                  << " ns (count " << a << "), local then merge " << static_cast<long>(local * 100) / 100.0
                  << " ns (count " << b << ")\n";
    }
    return 0;
}
