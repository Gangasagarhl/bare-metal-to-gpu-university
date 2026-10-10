// Measurement (F2-41): 20000 tasks of different sizes, run (a) one after another on the main
// thread, (b) on ThreadPools of 1, 2, 4 and 8 workers, (c) with one std::async per task.
// Built with -O2, no sanitizers, by run.sh.
#include <algorithm>
#include <chrono>
#include <cstdint>
#include <future>
#include <iostream>
#include <vector>

#include "thread_pool.hpp"

using Clock = std::chrono::steady_clock;

// A task that does `work` units of arithmetic and returns a checksum.
std::uint64_t job(std::uint64_t seed, int work)
{
    std::uint64_t x = seed;
    for (int i = 0; i < work; ++i) {
        x = x * 6364136223846793005ULL + 1442695040888963407ULL;
    }
    return x >> 60;
}

template <typename F>
double medianMs(F f)
{
    std::vector<double> v;
    for (int r = 0; r < 3; ++r) {
        const auto t0 = Clock::now();
        f();
        v.push_back(std::chrono::duration<double, std::milli>(Clock::now() - t0).count());
    }
    std::sort(v.begin(), v.end());
    return v[1];
}

int main()
{
    const int tasks = 20000;
    std::cout << tasks << " tasks; median of 3 runs; times in ms\n";
    for (int work : {10, 1000, 20000}) {
        std::uint64_t check = 0;
        const double serial = medianMs([&] {
            check = 0;
            for (int i = 0; i < tasks; ++i) {
                check += job(i, work);
            }
        });
        std::cout << "work " << work << " per task: serial " << static_cast<long>(serial);
        for (std::size_t workers : {1u, 2u, 4u, 8u}) {
            std::uint64_t sum = 0;
            const double ms = medianMs([&] {
                ThreadPool pool(workers);
                std::vector<std::future<std::uint64_t>> fs;
                fs.reserve(tasks);
                for (int i = 0; i < tasks; ++i) {
                    fs.push_back(pool.submit(job, static_cast<std::uint64_t>(i), work));
                }
                sum = 0;
                for (auto& f : fs) {
                    sum += f.get();
                }
            });
            std::cout << ", pool(" << workers << ") " << static_cast<long>(ms) << (sum == check ? "" : " WRONG");
        }
        if (work <= 1000) {
            std::uint64_t sum = 0;
            const double ms = medianMs([&] {
                std::vector<std::future<std::uint64_t>> fs;
                for (int i = 0; i < tasks; ++i) {
                    fs.push_back(std::async(std::launch::async, job, static_cast<std::uint64_t>(i), work));
                }
                sum = 0;
                for (auto& f : fs) {
                    sum += f.get();
                }
            });
            std::cout << ", async-per-task " << static_cast<long>(ms) << (sum == check ? "" : " WRONG");
        }
        std::cout << '\n';
    }
    return 0;
}
