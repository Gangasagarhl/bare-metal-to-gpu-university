// F1-30 Listing 1: use several cores from C++. Each thread sums its own slice of an array
// into its own variable; a shared counter is updated with an atomic operation.
#include <atomic>
#include <cstdio>
#include <numeric>
#include <thread>
#include <vector>

int main()
{
    std::printf("std::thread::hardware_concurrency() on this machine: %u\n",
                std::thread::hardware_concurrency());
    const std::size_t n = 1'000'000;
    std::vector<long> data(n);
    std::iota(data.begin(), data.end(), 1L);          // 1, 2, 3, ..., n
    const int workers = 4;
    std::vector<long> partial(workers, 0);            // one result slot per thread
    std::atomic<long> chunksDone{0};                  // shared, so it must be atomic
    std::vector<std::thread> pool;
    for (int w = 0; w < workers; ++w) {
        pool.emplace_back([&, w] {
            const std::size_t begin = n * w / workers, end = n * (w + 1) / workers;
            long s = 0;                               // private: no other thread touches it
            for (std::size_t i = begin; i < end; ++i) {
                s += data[i];
            }
            partial[w] = s;                           // each thread writes only its own slot
            chunksDone.fetch_add(1);                  // one indivisible read-modify-write
        });
    }
    for (std::thread& t : pool) {
        t.join();                                     // wait: after join, results are visible
    }
    const long total = std::accumulate(partial.begin(), partial.end(), 0L);
    std::printf("threads %d, chunks done %ld, total %ld, expected %ld\n", workers,
                chunksDone.load(), total, static_cast<long>(n) * (static_cast<long>(n) + 1) / 2);
    return total == static_cast<long>(n) * (static_cast<long>(n) + 1) / 2 ? 0 : 1;
}
