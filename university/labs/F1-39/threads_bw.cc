// Read bandwidth with 1, 2 and 4 threads: each thread sums its own quarter, half or all of a
// 512 MiB block, so the total bytes read are the same in every row.
// Numbers are measurements on the machine that ran this program, not specifications.
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <thread>
#include <vector>

int main()
{
    const std::size_t n = (512u * 1024 * 1024) / sizeof(std::uint64_t);
    const std::vector<std::uint64_t> data(n, 1);
    std::printf("%8s %10s %10s\n", "threads", "ms", "GB/s");
    for (const std::size_t threads : {1u, 2u, 4u}) {
        std::vector<std::uint64_t> sums(threads * 8, 0);      // 8 apart: no false sharing
        const auto t0 = std::chrono::steady_clock::now();
        std::vector<std::thread> pool;
        for (std::size_t t = 0; t < threads; ++t) {
            pool.emplace_back([&data, &sums, t, threads, n] {
                std::uint64_t s = 0;
                for (std::size_t r = 0; r < 4; ++r) {         // four passes over its part
                    for (std::size_t i = t * n / threads; i < (t + 1) * n / threads; ++i) {
                        s += data[i];
                    }
                }
                sums[t * 8] = s;
            });
        }
        for (std::thread& th : pool) {
            th.join();
        }
        const auto t1 = std::chrono::steady_clock::now();
        std::uint64_t total = 0;
        for (const std::uint64_t s : sums) {
            total += s;
        }
        const double ns = std::chrono::duration<double, std::nano>(t1 - t0).count();
        const double bytes = 4.0 * double(n * sizeof(std::uint64_t));
        std::printf("%8zu %10.1f %10.2f   (sum %llu)\n", threads, ns / 1e6, bytes / ns,
                    static_cast<unsigned long long>(total));
    }
    return 0;
}
