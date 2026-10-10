// F6-13 Listing 3: how many atomic operations land on ONE address, per warp and per kernel.
// Counts operations, not time: the serialisation cost per operation depends on the GPU
// (not stated here; see the unverified box).
#include <algorithm>
#include <cstdio>

int main()
{
    const long n = 1L << 24;
    long perThread = 0, perWarp = 0, perBlock = 0;
    int busiestWarp = 0;
    for (long w = 0; w < n / 32; ++w) {                          // every warp of the count kernel
        int pass = 0;
        for (int lane = 0; lane < 32; ++lane) {
            long i = w * 32 + lane;
            long v = (i * 2654435761u) % 1000003;                 // same data as Listing 1
            pass += (v % 3 == 0);
        }
        perThread += pass;                                        // countAtomic: one per passing thread
        perWarp += (pass > 0);                                    // countWarpAgg: one per warp with a pass
        busiestWarp = std::max(busiestWarp, pass);
    }
    perBlock = n / 256;                                           // countBlockAgg: one per block
    std::printf("elements: %ld; warps: %ld; blocks of 256: %ld\n", n, n / 32, n / 256);
    std::printf("%-34s %s\n", "version", "atomics on the one global counter");
    std::printf("%-34s %ld\n", "countAtomic (one per passing thread)", perThread);
    std::printf("%-34s %ld\n", "countWarpAgg (one per warp)", perWarp);
    std::printf("%-34s %ld\n", "countBlockAgg (one per block)", perBlock);
    std::printf("most passing lanes in one warp: %d of 32\n\n", busiestWarp);

    std::printf("one warp instruction, 32 lanes: operations on the busiest address\n");
    std::printf("%-40s %d\n", "all lanes -> counter[0]", 32);
    std::printf("%-40s %d\n", "lane -> counter[lane] (32 addresses)", 1);
    std::printf("%-40s %d\n", "lane -> bin[lane % 4] (4 bins)", 8);
    std::printf("%-40s %d\n", "warp-aggregated (leader only)", 1);
    return 0;
}
