// ftl.cpp - F1-49 Listing 1: erase-before-write, out-of-place writes and
// write amplification in the university's tiny FTL simulator (ftl.h).
// Sizes are made up for teaching: 8 blocks of 4 pages, 24 logical pages.
#include "ftl.h"

#include <cstdio>

static void report(const char* name, const TinyFtl& ftl)
{
    const FlashStats& s = ftl.stats();
    std::printf("%-22s host writes %4ld  flash writes %4ld  gc copies %4ld  erases %3ld  WA %.2f\n",
                name, s.hostWrites, s.flashWrites, s.gcCopies, s.erases,
                static_cast<double>(s.flashWrites) / static_cast<double>(s.hostWrites));
}

int main()
{
    const int blocks = 8;
    const int pagesPerBlock = 4;
    const int logicalPages = 24;   // 32 physical pages, 24 offered to the host

    // 1. The flash rule: no overwrite in place.
    TinyFtl raw(blocks, pagesPerBlock, logicalPages);
    std::printf("program page 5 (erased): %s\n", raw.program(5) ? "ok" : "refused");
    std::printf("program page 5 again   : %s\n", raw.program(5) ? "ok" : "refused");

    // 2. Out-of-place update: the same logical page moves to a new physical page.
    TinyFtl ftl(blocks, pagesPerBlock, logicalPages);
    for (int v = 1; v <= 3; ++v) {
        ftl.write(7);
        std::printf("write %d of logical page 7 -> physical page %d\n", v, *ftl.lookup(7));
    }

    // 3. Sequential overwrites: whole blocks become invalid together.
    TinyFtl seq(blocks, pagesPerBlock, logicalPages);
    for (int round = 0; round < 10; ++round) {
        for (int lpn = 0; lpn < logicalPages; ++lpn) {
            seq.write(lpn);
        }
    }
    report("sequential, 10 rounds", seq);

    // 4. Random overwrites after a full first fill.
    TinyFtl rnd(blocks, pagesPerBlock, logicalPages);
    for (int lpn = 0; lpn < logicalPages; ++lpn) {
        rnd.write(lpn);
    }
    std::uint32_t seed = 49;
    for (int i = 0; i < 216; ++i) {
        rnd.write(static_cast<int>(nextRandom(seed) % logicalPages));
    }
    report("random, same volume", rnd);

    std::printf("erase count per block (random run):");
    for (int e : rnd.eraseCounts()) {
        std::printf(" %d", e);
    }
    std::printf("\n");
    return 0;
}
