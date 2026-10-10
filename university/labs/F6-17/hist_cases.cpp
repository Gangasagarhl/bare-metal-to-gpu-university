// F6-17 Listing 3: E4's acceptance cases for the histogram, on the CPU. Replays the privatized
// kernel of Listing 1 (histStride: a fixed grid of blocks walks the input; each block counts into its
// own 256 bins, then adds them to the global bins) block by block and compares with a plain count.
// Sizes 1, 2^k - 1, 2^k + 1 and four input kinds, as milestone E4 asks.
#include <cstdint>
#include <cstdio>
#include <random>
#include <vector>

constexpr std::size_t BINS = 256, BLOCK = 256, GRID = 7;      // a small odd grid on purpose

std::vector<std::uint32_t> histStrideReplay(const std::vector<std::uint8_t>& in)
{
    std::vector<std::uint32_t> global(BINS, 0);
    for (std::size_t b = 0; b < GRID; ++b) {
        std::vector<std::uint32_t> local(BINS, 0);                            // 1. zero
        for (std::size_t t = 0; t < BLOCK; ++t) {                             // 2. count
            for (std::size_t i = b * BLOCK + t; i < in.size(); i += GRID * BLOCK) { ++local[in[i]]; }
        }
        for (std::size_t v = 0; v < BINS; ++v) { global[v] += local[v]; }    // 3. merge
    }
    return global;
}

int main()
{
    std::mt19937 rng(17);
    const std::size_t sizes[] = {1, 2, 255, 257, 1791, 1793, 65535, 65537, 1000003};
    const char* kinds[] = {"random", "sorted", "reverse", "all-equal"};
    int failures = 0;
    std::printf("%-9s", "n");
    for (const char* k : kinds) { std::printf(" %-10s", k); }
    std::printf("\n");
    for (std::size_t n : sizes) {
        std::printf("%-9zu", n);
        for (int k = 0; k < 4; ++k) {
            std::vector<std::uint8_t> in(n);
            for (std::size_t i = 0; i < n; ++i) {
                in[i] = (k == 0) ? static_cast<std::uint8_t>(rng())
                      : (k == 1) ? static_cast<std::uint8_t>(i * 256 / n)
                      : (k == 2) ? static_cast<std::uint8_t>(255 - i * 256 / n) : std::uint8_t{42};
            }
            std::vector<std::uint32_t> ref(BINS, 0);
            for (std::uint8_t v : in) { ++ref[v]; }
            bool ok = histStrideReplay(in) == ref;
            failures += !ok;
            std::printf(" %-10s", ok ? "pass" : "FAIL");
        }
        std::printf("\n");
    }
    std::printf("failures: %d\n", failures);
    return failures == 0 ? 0 : 1;
}
