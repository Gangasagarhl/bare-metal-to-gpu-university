// F6-17 Listing 2: where the atomic operations of a 256-bin byte histogram go, counted exactly
// for three inputs and the three kernels of Listing 1 (n = 2^26 bytes, blocks of 256 threads).
// "busiest" = the most operations that hit one single global address over the whole kernel.
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <vector>

constexpr std::uint64_t N = std::uint64_t{1} << 26, BLOCK = 256, BINS = 256, STRIDE_GRID = 320;

unsigned sample(int kind, std::uint64_t i)             // the same data as Listing 1
{
    std::uint32_t r = static_cast<std::uint32_t>(i * 2654435761u);
    if (kind == 2) { return 7; }
    if (kind == 1 && r % 10 != 0) { return 0; }
    return r >> 24;
}

int main()
{
    const char* names[] = {"uniform", "skewed (90 % zeros)", "all bytes equal"};
    std::printf("%-20s %-28s %-15s %s\n", "input", "kernel", "global atomics", "busiest address");
    for (int kind = 0; kind < 3; ++kind) {
        std::vector<std::uint64_t> count(BINS, 0), blocksUsing(BINS, 0), lastBlock(BINS, ~std::uint64_t{0});
        std::vector<std::uint8_t> strideSeen(STRIDE_GRID * BINS, 0);
        std::vector<std::uint64_t> local(BINS, 0);
        std::uint64_t maxShared = 0;
        for (std::uint64_t i = 0; i < N; ++i) {
            unsigned v = sample(kind, i);
            ++count[v];                                    // histGlobal: one global atomic per byte
            std::uint64_t b = i / BLOCK;                   // histShared: block b counts element i
            if (lastBlock[v] != b) { lastBlock[v] = b; ++blocksUsing[v]; }
            strideSeen[((i / BLOCK) % STRIDE_GRID) * BINS + v] = 1;   // histStride: owner block
            ++local[v];
            if ((i + 1) % BLOCK == 0) {
                maxShared = std::max(maxShared, *std::max_element(local.begin(), local.end()));
                std::fill(local.begin(), local.end(), 0);
            }
        }
        std::uint64_t sharedMerges = 0, sharedBusiest = 0, strideMerges = 0, strideBusiest = 0;
        for (std::uint64_t v = 0; v < BINS; ++v) {
            sharedMerges += blocksUsing[v];
            sharedBusiest = std::max(sharedBusiest, blocksUsing[v]);
            std::uint64_t c = 0;
            for (std::uint64_t b = 0; b < STRIDE_GRID; ++b) { c += strideSeen[b * BINS + v]; }
            strideMerges += c;
            strideBusiest = std::max(strideBusiest, c);
        }
        auto row = [](const char* in, const char* k, std::uint64_t a, std::uint64_t b) {
            std::printf("%-20s %-28s %-15llu %llu\n", in, k, static_cast<unsigned long long>(a),
                        static_cast<unsigned long long>(b));
        };
        row(names[kind], "histGlobal", N, *std::max_element(count.begin(), count.end()));
        row("", "histShared (262144 blocks)", sharedMerges, sharedBusiest);
        row("", "histStride (320 blocks)", strideMerges, strideBusiest);
        std::printf("%-20s shared memory: busiest bin inside one block of 256 gets %llu updates\n", "",
                    static_cast<unsigned long long>(maxShared));
    }
    return 0;
}
