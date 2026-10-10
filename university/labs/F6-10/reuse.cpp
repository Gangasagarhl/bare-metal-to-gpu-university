// F6-10 Listing 2: how many global-memory reads a 1-D stencil block needs,
// with and without staging in shared memory (pure arithmetic, no GPU needed).
#include <cstdio>
#include <initializer_list>

int main()
{
    std::printf("%-6s %-7s %-14s %-14s %-8s %s\n", "block", "radius", "naive reads", "tiled reads",
                "ratio", "tile bytes (int)");
    for (int block : {128, 256, 512}) {
        for (int radius : {1, 3, 8, 32}) {
            long naive = static_cast<long>(block) * (2 * radius + 1);  // every thread reads 2R+1 values
            long tiled = block + 2 * radius;                           // each value once, plus the halo
            double ratio = static_cast<double>(naive) / static_cast<double>(tiled);
            std::printf("%-6d %-7d %-14ld %-14ld %-8.2f %ld\n", block, radius, naive, tiled, ratio,
                        tiled * 4);
        }
    }
    return 0;
}
