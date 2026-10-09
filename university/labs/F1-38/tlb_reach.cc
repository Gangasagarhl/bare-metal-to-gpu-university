// TLB reach, measured: visit P cache blocks in a shuffled cycle. "spread": every block is in a
// different 4 KiB page. "packed": the same number of blocks side by side (few pages).
// "spread+huge": spread, in memory for which 2 MiB pages were requested with madvise.
// Times are measurements on the machine that ran this program, not specifications.
#include <chrono>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <random>
#include <string>
#include <sys/mman.h>
#include <utility>
#include <vector>

double nsPerLoad(std::size_t* mem, std::size_t blocks, bool spread)
{
    std::vector<std::size_t> order(blocks);
    for (std::size_t i = 0; i < blocks; ++i) {
        order[i] = i;
    }
    std::mt19937_64 rng(7);
    for (std::size_t i = blocks - 1; i > 0; --i) {                  // shuffle the visit order
        std::uniform_int_distribution<std::size_t> pick(0, i - 1);
        std::swap(order[i], order[pick(rng)]);
    }
    // word index of block b: one block per 4 KiB page (at a varying 64-byte slot, so the
    // blocks still spread over all cache sets), or all blocks side by side
    auto slot = [spread](std::size_t b) { return spread ? b * 512 + (b % 64) * 8 : b * 8; };
    for (std::size_t i = 0; i < blocks; ++i) {
        mem[slot(order[i])] = slot(order[(i + 1) % blocks]);
    }
    const std::size_t loads = 4'000'000;
    std::size_t p = slot(order[0]);
    for (std::size_t i = 0; i < blocks; ++i) {
        p = mem[p];                                                 // warm-up
    }
    const auto t0 = std::chrono::steady_clock::now();
    for (std::size_t i = 0; i < loads; ++i) {
        p = mem[p];
    }
    const auto t1 = std::chrono::steady_clock::now();
    if (p == 1) {
        std::printf("impossible\n");
    }
    return std::chrono::duration<double, std::nano>(t1 - t0).count() / loads;
}

int main()
{
    const std::size_t maxBlocks = 32768;
    const std::size_t bytes = maxBlocks * 4096;                      // 128 MiB
    const std::size_t hugePage = 2u * 1024 * 1024;
    auto* plain = static_cast<std::size_t*>(std::aligned_alloc(hugePage, bytes));
    auto* huge = static_cast<std::size_t*>(std::aligned_alloc(hugePage, bytes));
    if (plain == nullptr || huge == nullptr) {
        return 1;
    }
    madvise(plain, bytes, MADV_NOHUGEPAGE);                          // force 4 KiB pages
    const int adviceResult = madvise(huge, bytes, MADV_HUGEPAGE);    // ask for 2 MiB pages
    std::printf("madvise(MADV_HUGEPAGE) returned %d\n", adviceResult);
    std::printf("%8s %12s %12s %16s\n", "blocks", "spread ns", "packed ns", "spread+huge ns");
    for (std::size_t blocks = 16; blocks <= maxBlocks; blocks *= 2) {
        const double spread = nsPerLoad(plain, blocks, true);
        const double packed = nsPerLoad(plain, blocks, false);
        const double spreadHuge = nsPerLoad(huge, blocks, true);
        std::printf("%8zu %12.2f %12.2f %16.2f\n", blocks, spread, packed, spreadHuge);
    }
    std::ifstream rollup("/proc/self/smaps_rollup");                // were 2 MiB pages used?
    std::string line;
    while (std::getline(rollup, line)) {
        if (line.rfind("AnonHugePages:", 0) == 0) {
            std::printf("kernel report for this process: %s\n", line.c_str());
        }
    }
    std::free(plain);
    std::free(huge);
    return 0;
}
