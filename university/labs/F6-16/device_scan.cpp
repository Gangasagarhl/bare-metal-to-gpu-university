// F6-16 Listing 2: device-wide scan on the CPU, two ways, tested on the E4 sizes.
//   reduceThenScan : pass 1 tile sums; pass 2 scan of the tile sums; pass 3 scan each tile + offset
//   lookback       : ONE pass; "blocks" are std::threads that take tile numbers from an atomic counter
//                    and publish a status word per tile: X (not ready), A (aggregate ready),
//                    P (inclusive prefix ready). A tile looks back over its predecessors until it
//                    meets a P, adding the A values it passes (decoupled look-back).
#include <atomic>
#include <cstdint>
#include <cstdio>
#include <numeric>
#include <random>
#include <thread>
#include <vector>

constexpr int TILE = 256;
constexpr std::uint64_t X = 0, A = 1, P = 2;               // status in the top 2 bits

std::uint64_t pack(std::uint64_t status, std::uint64_t value) { return (status << 62) | value; }
std::uint64_t statusOf(std::uint64_t w) { return w >> 62; }
std::uint64_t valueOf(std::uint64_t w) { return w & ((std::uint64_t{1} << 62) - 1); }

std::vector<std::uint64_t> reduceThenScan(const std::vector<std::uint64_t>& in)
{
    const std::size_t n = in.size(), tiles = (n + TILE - 1) / TILE;
    std::vector<std::uint64_t> sums(tiles, 0), out(n);
    for (std::size_t t = 0; t < tiles; ++t) {               // pass 1 (one block per tile)
        for (std::size_t i = t * TILE; i < n && i < (t + 1) * TILE; ++i) { sums[t] += in[i]; }
    }
    std::exclusive_scan(sums.begin(), sums.end(), sums.begin(), std::uint64_t{0});   // pass 2
    for (std::size_t t = 0; t < tiles; ++t) {               // pass 3
        std::uint64_t run = sums[t];
        for (std::size_t i = t * TILE; i < n && i < (t + 1) * TILE; ++i) { run += in[i]; out[i] = run; }
    }
    return out;
}

std::vector<std::uint64_t> lookback(const std::vector<std::uint64_t>& in, int workers)
{
    const std::size_t n = in.size(), tiles = (n + TILE - 1) / TILE;
    std::vector<std::uint64_t> out(n);
    std::vector<std::atomic<std::uint64_t>> status(tiles);
    for (auto& s : status) { s.store(pack(X, 0)); }
    std::atomic<std::size_t> nextTile{0};
    auto block = [&]() {
        for (;;) {
            std::size_t t = nextTile.fetch_add(1);           // dynamic tile numbers: tile t-1 started first
            if (t >= tiles) { return; }
            std::uint64_t agg = 0;
            for (std::size_t i = t * TILE; i < n && i < (t + 1) * TILE; ++i) { agg += in[i]; }
            std::uint64_t exclusive = 0;
            if (t == 0) {
                status[0].store(pack(P, agg), std::memory_order_release);
            } else {
                status[t].store(pack(A, agg), std::memory_order_release);   // publish my aggregate
                std::size_t p = t;
                while (p > 0) {                               // look back
                    --p;
                    std::uint64_t w;
                    while (statusOf(w = status[p].load(std::memory_order_acquire)) == X) {
                        std::this_thread::yield();            // predecessor still working
                    }
                    exclusive += valueOf(w);
                    if (statusOf(w) == P) { break; }          // found an inclusive prefix: done
                }
                status[t].store(pack(P, exclusive + agg), std::memory_order_release);
            }
            std::uint64_t run = exclusive;
            for (std::size_t i = t * TILE; i < n && i < (t + 1) * TILE; ++i) { run += in[i]; out[i] = run; }
        }
    };
    std::vector<std::thread> pool;
    for (int w = 0; w < workers; ++w) { pool.emplace_back(block); }
    for (auto& th : pool) { th.join(); }
    return out;
}

int main()
{
    const std::size_t sizes[] = {1, 2, 255, 256, 257, (1u << 16) - 1, (1u << 16) + 1, 1000000};
    const char* kinds[] = {"random", "sorted", "reverse", "all-equal"};
    std::mt19937 rng(7);
    int failures = 0;
    std::printf("%-9s %-10s %-7s %-16s %s\n", "n", "input", "tiles", "reduce-then-scan", "look-back (4 threads)");
    for (std::size_t n : sizes) {
        for (int k = 0; k < 4; ++k) {
            std::vector<std::uint64_t> in(n);
            for (std::size_t i = 0; i < n; ++i) {
                in[i] = (k == 0) ? rng() % 1000 : (k == 1) ? i % 1000 : (k == 2) ? (n - i) % 1000 : 5;
            }
            std::vector<std::uint64_t> ref(n);
            std::inclusive_scan(in.begin(), in.end(), ref.begin());
            bool ok1 = reduceThenScan(in) == ref;
            bool ok2 = lookback(in, 4) == ref;
            failures += !ok1 + !ok2;
            std::printf("%-9zu %-10s %-7zu %-16s %s\n", n, kinds[k], (n + TILE - 1) / TILE,
                        ok1 ? "pass" : "FAIL", ok2 ? "pass" : "FAIL");
        }
    }
    std::printf("failures: %d\n", failures);
    return failures == 0 ? 0 : 1;
}
