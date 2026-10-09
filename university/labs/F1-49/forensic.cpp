// forensic.cpp - F1-49 forensic evidence generator: "the SSD that got slow when full".
// Uses the university's TinyFtl (ftl.h). Prints one log line per phase, in the
// simulator's own format. Sizes are made up: 16 blocks of 8 pages = 128 physical pages.
#include "ftl.h"

#include <cstdio>

static void phase(const char* name, TinyFtl& ftl, int usedPages, int writes, std::uint32_t& seed)
{
    const FlashStats before = ftl.stats();
    for (int i = 0; i < writes; ++i) {
        ftl.write(static_cast<int>(nextRandom(seed) % static_cast<std::uint32_t>(usedPages)));
    }
    const FlashStats& s = ftl.stats();
    const long host = s.hostWrites - before.hostWrites;
    const long flash = s.flashWrites - before.flashWrites;
    const long gc = s.gcCopies - before.gcCopies;
    std::printf("%-34s host=%4ld flash=%4ld gc_copies=%4ld erases=%3ld flash_per_host=%.2f\n",
                name, host, flash, gc, s.erases - before.erases,
                static_cast<double>(flash) / static_cast<double>(host));
}

int main()
{
    const int blocks = 16;
    const int ppb = 8;
    const int logical = 112;   // the host sees 112 of the 128 physical pages
    std::uint32_t seed = 2026;

    TinyFtl ssd(blocks, ppb, logical);
    std::printf("drive: %d logical pages offered, %d physical pages\n", logical, blocks * ppb);

    for (int lpn = 0; lpn < 56; ++lpn) {
        ssd.write(lpn);
    }
    phase("day 1: 50 % of logical space used", ssd, 56, 800, seed);

    for (int lpn = 56; lpn < 106; ++lpn) {
        ssd.write(lpn);
    }
    phase("day 9: 95 % of logical space used", ssd, 106, 800, seed);

    // The user deletes files covering pages 30..105, but the file system sends no TRIM:
    // the drive still believes those pages hold data and keeps copying them.
    phase("day 10: files deleted, no TRIM", ssd, 30, 800, seed);

    for (int lpn = 30; lpn < 106; ++lpn) {
        ssd.trim(lpn);
    }
    phase("day 11: same files, TRIM sent", ssd, 30, 800, seed);
    return 0;
}
