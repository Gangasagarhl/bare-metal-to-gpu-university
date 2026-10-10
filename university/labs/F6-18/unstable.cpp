// F6-18 forensic evidence: the same LSD radix sort, but in the scenario's version step 3 hands out
// places inside a tile with an atomic counter per digit, so keys with the same digit leave a tile in
// the order the atomics happened to run, not in their input order. The replay draws that order
// with a seeded generator. Every pass is still a correct sort by ITS digit.
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <numeric>
#include <random>
#include <vector>

constexpr int TILE = 256, BITS = 4, DIGITS = 1 << BITS;

void sortPass(std::vector<std::uint32_t>& keys, int shift, bool stable, std::mt19937& rng)
{
    const std::size_t n = keys.size(), tiles = (n + TILE - 1) / TILE;
    std::vector<std::size_t> offset(DIGITS * tiles, 0);
    for (std::size_t i = 0; i < n; ++i) { ++offset[((keys[i] >> shift) & (DIGITS - 1)) * tiles + i / TILE]; }
    std::exclusive_scan(offset.begin(), offset.end(), offset.begin(), std::size_t{0});
    std::vector<std::uint32_t> out(n);
    for (std::size_t t = 0; t < tiles; ++t) {
        std::vector<std::size_t> order;
        for (std::size_t i = t * TILE; i < n && i < (t + 1) * TILE; ++i) { order.push_back(i); }
        if (!stable) { std::shuffle(order.begin(), order.end(), rng); }    // atomics' arrival order
        for (std::size_t i : order) { out[offset[((keys[i] >> shift) & (DIGITS - 1)) * tiles + t]++] = keys[i]; }
    }
    keys.swap(out);
}

std::size_t descents(const std::vector<std::uint32_t>& k, int shift, int bits)
{
    std::size_t d = 0;
    const std::uint32_t mask = (bits >= 32) ? 0xffffffffu : ((1u << bits) - 1);
    for (std::size_t i = 1; i < k.size(); ++i) { d += ((k[i - 1] >> shift) & mask) > ((k[i] >> shift) & mask); }
    return d;
}

int main()
{
    const std::size_t n = 100000;
    for (bool stable : {true, false}) {
        std::mt19937 data(3), sched(99);
        std::vector<std::uint32_t> keys(n);
        for (auto& k : keys) { k = static_cast<std::uint32_t>(data()) & 0xffffu; }   // 16-bit keys: 4 passes
        std::printf("%s scatter\n", stable ? "stable (input order kept)" : "unstable (atomic order)");
        std::printf("  %-6s %-30s %s\n", "pass", "out-of-order neighbours by", "out-of-order neighbours by");
        std::printf("  %-6s %-30s %s\n", "", "this pass's digit", "all digits sorted so far");
        for (int pass = 0; pass < 4; ++pass) {
            sortPass(keys, pass * BITS, stable, sched);
            std::printf("  %-6d %-30zu %zu\n", pass, descents(keys, pass * BITS, BITS),
                        descents(keys, 0, (pass + 1) * BITS));
        }
        std::printf("  first 8 keys: ");
        for (int i = 0; i < 8; ++i) { std::printf(" %u", keys[i]); }
        std::printf("\n");
    }
    return 0;
}
