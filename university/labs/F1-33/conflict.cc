// Conflict misses, measured: k blocks visited in a cycle, placed `spacing` bytes apart.
// With a spacing equal to (number of sets x line size) every block falls into the SAME set.
// Times are measurements on the machine that ran this program, not specifications.
#include <chrono>
#include <cstddef>
#include <cstdio>
#include <vector>

double nsPerLoad(std::vector<std::size_t>& mem, std::size_t spacingBytes, std::size_t k)
{
    const std::size_t step = spacingBytes / sizeof(std::size_t);    // spacing in elements
    for (std::size_t j = 0; j < k; ++j) {
        mem[j * step] = ((j + 1) % k) * step;                        // link block j to block j+1
    }
    const std::size_t loads = 20'000'000;
    std::size_t p = 0;
    for (std::size_t i = 0; i < 1000; ++i) {                         // warm-up
        p = mem[p];
    }
    const auto t0 = std::chrono::steady_clock::now();
    for (std::size_t i = 0; i < loads; ++i) {
        p = mem[p];                                                  // dependent loads
    }
    const auto t1 = std::chrono::steady_clock::now();
    if (p >= mem.size()) {
        std::printf("impossible\n");                                 // keeps p alive
    }
    return std::chrono::duration<double, std::nano>(t1 - t0).count() / loads;
}

int main()
{
    std::vector<std::size_t> mem(8u * 1024 * 1024);                  // 64 MiB of zeros
    std::printf("%8s", "blocks");
    for (const std::size_t spacing : {4096u, 4160u}) {
        std::printf("  ns/load @ %zu B apart", spacing);
    }
    std::printf("\n");
    for (const std::size_t k : {4u, 8u, 10u, 12u, 13u, 14u, 16u, 24u, 32u}) {
        std::printf("%8zu", k);
        for (const std::size_t spacing : {4096u, 4160u}) {
            std::printf("  %21.2f", nsPerLoad(mem, spacing, k));
        }
        std::printf("\n");
    }
    return 0;
}
