// Predict, then check: how many misses does each loop order cause in a small cache?
// The matrix is n x n doubles stored row by row starting at address 0; the cache model is
// the one from F1-33 (cache.hpp): 8 sets x 2 ways x 64-byte lines = 1 KiB.
#include <cstdint>
#include <cstdio>

#include "cache.hpp"

std::uint64_t missesFor(unsigned n, bool rowsFirst)
{
    Cache cache(8, 2, 64);
    for (unsigned a = 0; a < n; ++a) {
        for (unsigned b = 0; b < n; ++b) {
            const unsigned i = rowsFirst ? a : b;
            const unsigned j = rowsFirst ? b : a;
            cache.access(std::uint64_t{i} * n * 8 + std::uint64_t{j} * 8);  // address of m[i][j]
        }
    }
    return cache.misses();
}

int main()
{
    std::printf("%6s %10s %14s %14s\n", "n", "accesses", "misses (rows)", "misses (cols)");
    for (const unsigned n : {8u, 16u, 64u}) {
        std::printf("%6u %10u %14llu %14llu\n", n, n * n,
                    static_cast<unsigned long long>(missesFor(n, true)),
                    static_cast<unsigned long long>(missesFor(n, false)));
    }
    return 0;
}
