// BR-02 Listing 3: a lock-step CPU model of the saxpyNeg kernel's vote and count.
// There is no GPU in the build container, so this model replays what each version of
// the kernel computes when lanes are grouped W at a time (W = 32 or 64), with lanes of a
// group taken from consecutive threadIdx.x values. It models the ARITHMETIC of the vote,
// not timing, memory or scheduling.
#include <bit>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <functional>
#include <initializer_list>
#include <vector>

enum class Version { cuda32, naivePort, fixed };

// The count one launch would produce: blocks of 256 threads, groups of w lanes.
static unsigned count(Version v, int w, int n, const std::function<bool(int)>& negative)
{
    const int block = 256;
    unsigned total = 0;
    for (int base = 0; base < n; base += w) {                 // one warp/wavefront at a time
        std::uint64_t ballot = 0;                             // what the hardware vote returns
        for (int lane = 0; lane < w; ++lane) {
            const int i = base + lane;
            if (i < n && negative(i)) { ballot |= std::uint64_t{1} << lane; }
        }
        for (int lane = 0; lane < w; ++lane) {                // every lane runs the leader test
            const int tid = (base + lane) % block;            // threadIdx.x
            if (v == Version::fixed) {
                if (tid % w == 0) { total += static_cast<unsigned>(std::popcount(ballot)); }
            } else {
                const auto mask = static_cast<std::uint32_t>(ballot);  // stored in 32 bits
                if ((tid & 31) == 0) { total += static_cast<unsigned>(std::popcount(mask)); }
            }
        }
    }
    return total;
}

static float value(std::uint32_t i, std::uint32_t seed)       // as in Listing 1
{
    std::uint32_t h = (i + seed) * 2654435761u;
    h ^= h >> 15;
    return static_cast<float>(h % 2000003u) / 1000.0f - 1000.0f;
}

int main()
{
    const float a = 1.7f;
    std::printf("blocks of 256 threads; count = what the kernel would store in *negatives\n");
    std::printf("%-9s %-33s %-23s %9s %9s  %s\n", "n", "data", "version", "count", "reference",
                "result");
    for (const int n : {1 << 20, 1000003}) {
        std::vector<bool> lab(n);
        for (int i = 0; i < n; ++i) {
            lab[i] = std::fma(a, value(i, 1), value(i, 2)) < 0.0f;  // Listing 1's reference
        }
        const struct { const char* name; std::function<bool(int)> neg; } data[] = {
            {"lab data (Listing 1 values)", [&](int i) { return static_cast<bool>(lab[i]); }},
            {"all results negative", [](int) { return true; }},
            {"negative only in lanes 32-63", [](int i) { return i % 64 >= 32; }},
            {"negative except i % 128 in 32-63",
             [](int i) { return i % 128 < 32 || i % 128 >= 64; }},
        };
        const struct { const char* name; Version v; int w; } runs[] = {
            {"CUDA original, W = 32", Version::cuda32, 32},
            {"naive port, W = 64", Version::naivePort, 64},
            {"fixed, W = 64", Version::fixed, 64},
            {"fixed, W = 32", Version::fixed, 32},
        };
        for (const auto& d : data) {
            unsigned ref = 0;
            for (int i = 0; i < n; ++i) { ref += d.neg(i) ? 1u : 0u; }
            for (const auto& r : runs) {
                const unsigned c = count(r.v, r.w, n, d.neg);
                std::printf("%-9d %-33s %-23s %9u %9u  %s\n", n, d.name, r.name, c, ref,
                            c == ref ? "ok" : "WRONG");
            }
        }
    }
    return 0;
}
