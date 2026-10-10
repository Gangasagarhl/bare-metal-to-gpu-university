// F6-17 forensic evidence: replays the privatized histogram of the scenario, whose kernel forgot
// step 1 (zeroing the shared bins). Shared memory is not cleared between blocks, so in this
// replay a block's bins start with whatever the previous block on the same SM left there.
// (On a GPU the starting content is simply undefined; this replay picks one repeatable case.)
// The toy GPU has 4 SMs; blocks go to SMs in turn and run one after another on each SM.
#include <cstdint>
#include <cstdio>
#include <vector>

constexpr int BINS = 256, BLOCK = 256, SMS = 4;

std::uint64_t run(std::uint64_t n, bool zeroFirst, std::vector<std::uint64_t>& bins)
{
    std::vector<std::vector<std::uint32_t>> shared(SMS, std::vector<std::uint32_t>(BINS, 0));
    bins.assign(BINS, 0);
    const std::uint64_t blocks = (n + BLOCK - 1) / BLOCK;
    for (std::uint64_t b = 0; b < blocks; ++b) {
        std::vector<std::uint32_t>& local = shared[b % SMS];
        if (zeroFirst) { local.assign(BINS, 0); }                          // step 1
        for (std::uint64_t i = b * BLOCK; i < n && i < (b + 1) * BLOCK; ++i) {
            ++local[(i * 2654435761u) >> 24 & 0xff];                       // step 2
        }
        for (int v = 0; v < BINS; ++v) { bins[v] += local[v]; }            // step 3 (merge)
    }
    std::uint64_t total = 0;
    for (std::uint64_t c : bins) { total += c; }
    return total;
}

int main()
{
    std::printf("%-10s %-8s %-18s %-18s %s\n", "n", "blocks", "total (bug)", "total (zeroed)", "bin 0 (bug / zeroed)");
    for (std::uint64_t n : {256ull, 1024ull, 4096ull, 65536ull, 1048576ull}) {
        std::vector<std::uint64_t> bad, good;
        std::uint64_t tb = run(n, false, bad);
        std::uint64_t tg = run(n, true, good);
        std::printf("%-10llu %-8llu %-18llu %-18llu %llu / %llu\n", static_cast<unsigned long long>(n),
                    static_cast<unsigned long long>((n + BLOCK - 1) / BLOCK), static_cast<unsigned long long>(tb),
                    static_cast<unsigned long long>(tg), static_cast<unsigned long long>(bad[0]),
                    static_cast<unsigned long long>(good[0]));
    }
    return 0;
}
