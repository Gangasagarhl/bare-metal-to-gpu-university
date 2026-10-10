// BR-02 Listing 4: what a block size chosen for 32-lane warps means for 64-lane
// wavefronts. Pure arithmetic: a block of B threads is cut into ceil(B / W) groups of
// W lanes; lanes of the last group beyond B do no work (they are idle, not free).
#include <cstdio>
#include <initializer_list>

int main()
{
    const int blockSizes[] = {32, 48, 64, 96, 128, 160, 192, 256};
    std::printf("%6s | %-24s | %-24s |\n", "block", "W = 32 (warps)", "W = 64 (wavefronts)");
    std::printf("%6s | %6s %7s %9s | %6s %7s %9s |\n", "B", "groups", "idle", "busy %",
                "groups", "idle", "busy %");
    for (const int b : blockSizes) {
        std::printf("%6d |", b);
        for (const int w : {32, 64}) {
            const int groups = (b + w - 1) / w;
            const int idle = groups * w - b;
            const double busy = 100.0 * b / (groups * w);
            std::printf(" %6d %7d %8.1f%% |", groups, idle, busy);
        }
        std::printf("\n");
    }
    return 0;
}
