// quorum.cpp - quorum reads and writes: when must a read see the latest write?
// A write is stored on W of the N replicas; a read asks R replicas and keeps the answer with
// the highest version number. The program tries EVERY possible write set and read set and
// counts how often the read set contains at least one replica that has the latest write.
#include <cstdio>
#include <vector>

int popcount(unsigned x)
{
    int c = 0;
    for (; x != 0; x &= x - 1) {
        ++c;
    }
    return c;
}

int main()
{
    for (int n : {3, 5}) {
        std::printf("N = %d replicas\n", n);
        std::printf("%3s %3s %8s %22s %14s\n", "W", "R", "R+W>N", "read sees latest write",
                    "combinations");
        const unsigned all = 1u << n;
        for (int w = 1; w <= n; ++w) {
            for (int r = 1; r <= n; ++r) {
                long total = 0;
                long fresh = 0;
                for (unsigned ws = 0; ws < all; ++ws) {
                    if (popcount(ws) != w) {
                        continue;
                    }
                    for (unsigned rs = 0; rs < all; ++rs) {
                        if (popcount(rs) != r) {
                            continue;
                        }
                        ++total;
                        fresh += (ws & rs) != 0 ? 1 : 0;   // overlap: some replica has it
                    }
                }
                std::printf("%3d %3d %8s %21.1f%% %14ld\n", w, r, r + w > n ? "yes" : "no",
                            100.0 * fresh / total, total);
            }
        }
        std::printf("\n");
    }
    return 0;
}
