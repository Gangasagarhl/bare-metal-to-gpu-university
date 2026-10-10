// HW203 practical (P), STARTING FILE: predict the cache behaviour of a loop nest, then measure.
// Complete the four functions marked TODO; change nothing else. The cache model is the course's
// own (cache.hpp from F1-33: tags only, exact LRU). Build with the course command:
//   g++ -std=c++20 -Wall -Wextra -Wpedantic -Werror -g -fsanitize=address,undefined
//       cache_exam_start.cpp -o cache_exam_start   (one line), then
//   ./cache_exam_start < cache_exam_start.in
// As handed out the program builds, prints wrong numbers and ends with "self-check: 7 failures".
// Input: one case per line, "sets ways lineBytes n elemBytes tile". For each case the program
// counts the accesses and misses of four loop nests over n x n row-major matrices:
//   sum by rows, sum by columns, naive transpose b[j][i] = a[i][j], blocked transpose (tile x tile).
// Matrix a starts at address 0, matrix b directly after it. Exit code 1 if the self-check fails.
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <iostream>

#include "cache.hpp"

struct Geometry
{
    unsigned sets, ways, lineBytes;
};

struct Result
{
    std::uint64_t accesses = 0;
    std::uint64_t misses = 0;
};

// Address of element [i][j] of an n-column row-major matrix starting at base.
std::uint64_t addressOf(std::uint64_t base, unsigned i, unsigned j, unsigned n, unsigned elemBytes)
{
    // TODO 1: row-major layout, elemBytes per element (F1-34 jargon box "Row-major order").
    (void)i;
    (void)j;
    (void)n;
    (void)elemBytes;
    return base;
}

Result sumRows(const Geometry& g, unsigned n, unsigned elemBytes)
{
    Cache cache(g.sets, g.ways, g.lineBytes);
    Result r;
    for (unsigned i = 0; i < n; ++i) {
        for (unsigned j = 0; j < n; ++j) {
            cache.access(addressOf(0, i, j, n, elemBytes));
            ++r.accesses;
        }
    }
    r.misses = cache.misses();
    return r;
}

Result sumCols(const Geometry& g, unsigned n, unsigned elemBytes)
{
    // TODO 2: the same two loops as sumRows with the row index innermost (stride n * elemBytes).
    (void)g;
    (void)n;
    (void)elemBytes;
    return Result{};
}

// b[j][i] = a[i][j]: one read of a, then one write of b, per element (both count as accesses).
Result transposeNaive(const Geometry& g, unsigned n, unsigned elemBytes)
{
    // TODO 3: for every i, j: access a[i][j] (base 0), then b[j][i] (base n * n * elemBytes);
    // count both accesses; misses from the one cache both matrices go through.
    (void)g;
    (void)n;
    (void)elemBytes;
    return Result{};
}

// The same transpose in tile x tile blocks: all of one block of a and b is finished before the next.
Result transposeBlocked(const Geometry& g, unsigned n, unsigned elemBytes, unsigned tile)
{
    // TODO 4: the same transpose in tile x tile blocks (F1-34 Layer 3, "Blocking"): loops over
    // ii and jj in steps of tile, then i and j inside the block (std::min for the last block).
    (void)g;
    (void)n;
    (void)elemBytes;
    (void)tile;
    return Result{};
}

// Known values: the worked example of F1-34 (8 sets x 2 ways x 64 B, doubles) and two
// consistency rules. The self-check does not know the exam cases' answers.
int selfCheck()
{
    int failures = 0;
    const Geometry g{8, 2, 64};
    struct Known
    {
        unsigned n;
        std::uint64_t rows, cols;
    };
    for (const Known k : {Known{8, 8, 8}, Known{16, 32, 256}, Known{64, 512, 4096}}) {
        const Result rows = sumRows(g, k.n, 8);
        const Result cols = sumCols(g, k.n, 8);
        if (rows.accesses != std::uint64_t{k.n} * k.n || rows.misses != k.rows) {
            std::printf("self-check: sum by rows, n = %u: %llu misses, expected %llu\n", k.n,
                        static_cast<unsigned long long>(rows.misses),
                        static_cast<unsigned long long>(k.rows));
            ++failures;
        }
        if (cols.accesses != std::uint64_t{k.n} * k.n || cols.misses != k.cols) {
            std::printf("self-check: sum by columns, n = %u: %llu misses, expected %llu\n", k.n,
                        static_cast<unsigned long long>(cols.misses),
                        static_cast<unsigned long long>(k.cols));
            ++failures;
        }
    }
    const Result naive = transposeNaive(g, 16, 8);
    const Result whole = transposeBlocked(g, 16, 8, 16);
    if (naive.accesses != 2 * 16 * 16 || naive.misses == 0) {
        std::printf("self-check: naive transpose n = 16 must make 512 accesses and some misses\n");
        ++failures;
    }
    if (whole.accesses != naive.accesses || whole.misses != naive.misses) {
        std::printf("self-check: a blocked transpose with tile = n must equal the naive one\n");
        ++failures;
    }
    std::printf("self-check: %d failures\n", failures);
    return failures;
}

int main()
{
    std::printf("%-5s %-4s %-4s %-5s %-4s %-4s %-9s %-9s %-9s %-9s %-9s\n", "sets", "ways", "line",
                "n", "elem", "tile", "accesses", "rows", "cols", "transp", "blocked");
    Geometry g{};
    unsigned n = 0;
    unsigned elemBytes = 0;
    unsigned tile = 0;
    while (std::cin >> g.sets >> g.ways >> g.lineBytes >> n >> elemBytes >> tile) {
        const Result rows = sumRows(g, n, elemBytes);
        const Result cols = sumCols(g, n, elemBytes);
        const Result naive = transposeNaive(g, n, elemBytes);
        const Result blocked = transposeBlocked(g, n, elemBytes, tile);
        std::printf("%-5u %-4u %-4u %-5u %-4u %-4u %-9llu %-9llu %-9llu %-9llu %-9llu\n", g.sets,
                    g.ways, g.lineBytes, n, elemBytes, tile,
                    static_cast<unsigned long long>(rows.accesses),
                    static_cast<unsigned long long>(rows.misses),
                    static_cast<unsigned long long>(cols.misses),
                    static_cast<unsigned long long>(naive.misses),
                    static_cast<unsigned long long>(blocked.misses));
    }
    std::printf("(misses of: sum by rows, sum by columns, naive transpose, blocked transpose;"
                " the transposes make twice the accesses shown)\n");
    return selfCheck() == 0 ? 0 : 1;
}
