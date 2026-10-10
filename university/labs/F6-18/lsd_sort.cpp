// F6-18 Listing 1 (lsd_sort.cpp): an LSD radix sort organised the way a GPU runs it (curriculum milestone E4).
// 32-bit keys (and key-value pairs), 4-bit digits, 8 passes. Each pass:
//   1. every tile of 256 keys counts its 16 digit values          (one block per tile)
//   2. one exclusive scan over the counts, digit-major:          offset[d][tile]
//   3. every tile writes its keys to offset[d][tile] + rank, keeping their order (stable)
// Signed and float keys are mapped to unsigned keys with the bit transformations that the CUB
// documentation lists, sorted, and mapped back.
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <numeric>
#include <random>
#include <vector>

constexpr int TILE = 256, BITS = 4, DIGITS = 1 << BITS;

void radixSortPairs(std::vector<std::uint32_t>& keys, std::vector<std::uint32_t>& vals)
{
    const std::size_t n = keys.size(), tiles = (n + TILE - 1) / TILE;
    std::vector<std::uint32_t> k2(n), v2(n);
    std::vector<std::size_t> offset(DIGITS * tiles);
    for (int shift = 0; shift < 32; shift += BITS) {
        std::fill(offset.begin(), offset.end(), 0);
        for (std::size_t t = 0; t < tiles; ++t) {                                   // 1. count
            for (std::size_t i = t * TILE; i < n && i < (t + 1) * TILE; ++i) {
                ++offset[((keys[i] >> shift) & (DIGITS - 1)) * tiles + t];
            }
        }
        std::exclusive_scan(offset.begin(), offset.end(), offset.begin(), std::size_t{0});   // 2. scan
        for (std::size_t t = 0; t < tiles; ++t) {                                   // 3. scatter
            for (std::size_t i = t * TILE; i < n && i < (t + 1) * TILE; ++i) {
                std::size_t dst = offset[((keys[i] >> shift) & (DIGITS - 1)) * tiles + t]++;
                k2[dst] = keys[i];
                v2[dst] = vals[i];
            }
        }
        keys.swap(k2);
        vals.swap(v2);
    }
}

std::uint32_t fromSigned(std::int32_t x) { return static_cast<std::uint32_t>(x) ^ 0x80000000u; }   // flip sign bit
std::uint32_t fromFloat(float f)
{
    std::uint32_t b;
    std::memcpy(&b, &f, sizeof b);
    return (b & 0x80000000u) ? ~b : (b ^ 0x80000000u);    // negative: invert all; positive: flip sign
}

int main()
{
    std::mt19937 rng(11);
    const std::size_t sizes[] = {1, 2, 255, 257, (1u << 16) - 1, (1u << 16) + 1, 1000003};
    const char* kinds[] = {"random", "sorted", "reverse", "all-equal"};
    int failures = 0;
    std::printf("%-9s %-10s %-12s %s\n", "n", "input", "keys sorted", "pairs stable (= std::stable_sort)");
    for (std::size_t n : sizes) {
        for (int k = 0; k < 4; ++k) {
            std::vector<std::uint32_t> keys(n), vals(n);
            for (std::size_t i = 0; i < n; ++i) {
                keys[i] = (k == 0) ? static_cast<std::uint32_t>(rng()) : (k == 1) ? static_cast<std::uint32_t>(i)
                        : (k == 2) ? static_cast<std::uint32_t>(n - i) : 42u;
                vals[i] = static_cast<std::uint32_t>(i);                        // original position
            }
            std::vector<std::size_t> idx(n);
            std::iota(idx.begin(), idx.end(), std::size_t{0});
            std::stable_sort(idx.begin(), idx.end(), [&](std::size_t a, std::size_t b) { return keys[a] < keys[b]; });
            std::vector<std::uint32_t> refK(n), refV(n);
            for (std::size_t i = 0; i < n; ++i) { refK[i] = keys[idx[i]]; refV[i] = vals[idx[i]]; }
            radixSortPairs(keys, vals);
            bool okK = keys == refK, okV = vals == refV;
            failures += !okK + !okV;
            std::printf("%-9zu %-10s %-12s %s\n", n, kinds[k], okK ? "pass" : "FAIL", okV ? "pass" : "FAIL");
        }
    }
    std::vector<std::int32_t> s = {5, -3, 0, -2147483647 - 1, 2147483647, -1, 3};
    std::vector<std::uint32_t> sk(s.size()), sv(s.size());
    for (std::size_t i = 0; i < s.size(); ++i) { sk[i] = fromSigned(s[i]); sv[i] = static_cast<std::uint32_t>(i); }
    radixSortPairs(sk, sv);
    std::printf("signed keys sorted:");
    for (std::uint32_t v : sv) { std::printf(" %d", s[v]); }
    std::vector<float> f = {2.5f, -0.5f, 0.0f, -7.25f, 1e9f, -1e-9f, 3.0f};
    std::vector<std::uint32_t> fk(f.size()), fv(f.size());
    for (std::size_t i = 0; i < f.size(); ++i) { fk[i] = fromFloat(f[i]); fv[i] = static_cast<std::uint32_t>(i); }
    radixSortPairs(fk, fv);
    std::printf("\nfloat keys sorted: ");
    for (std::uint32_t v : fv) { std::printf(" %g", static_cast<double>(f[v])); }
    std::printf("\nfailures: %d\n", failures);
    return failures == 0 ? 0 : 1;
}
