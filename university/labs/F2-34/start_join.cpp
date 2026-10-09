// Listing 1 (F2-34): start four threads, give each its own slice of the work, join them all.
#include <cstddef>
#include <iostream>
#include <numeric>
#include <thread>
#include <vector>

// Adds up data[begin, end) and writes the total into *out.
void sumSlice(const std::vector<long>& data, std::size_t begin, std::size_t end, long* out)
{
    long total = 0;
    for (std::size_t i = begin; i < end; ++i) {
        total += data[i];
    }
    *out = total;  // each thread writes only its own slot
}

int main()
{
    const std::size_t n = 1'000'000;
    std::vector<long> data(n);
    std::iota(data.begin(), data.end(), 1L);  // 1, 2, 3, ..., n

    const std::size_t numThreads = 4;
    std::vector<long> partial(numThreads, 0);
    std::vector<std::thread> workers;

    for (std::size_t t = 0; t < numThreads; ++t) {
        const std::size_t begin = n * t / numThreads;
        const std::size_t end = n * (t + 1) / numThreads;
        workers.emplace_back(sumSlice, std::cref(data), begin, end, &partial[t]);
    }
    for (std::thread& w : workers) {
        w.join();  // wait until this worker has finished
    }

    const long total = std::accumulate(partial.begin(), partial.end(), 0L);
    for (std::size_t t = 0; t < numThreads; ++t) {
        std::cout << "thread " << t << " summed " << partial[t] << '\n';
    }
    std::cout << "total      = " << total << '\n';
    std::cout << "n(n+1)/2   = " << static_cast<long>(n) * (static_cast<long>(n) + 1) / 2 << '\n';
    std::cout << "hardware_concurrency() on this machine: "
              << std::thread::hardware_concurrency() << '\n';
    return 0;
}
