// F6-36 Listing 3: what tiled_partition gives each thread, and the steps of a tile reduction.
// For a block of 64 threads cut into tiles of 16: thread_rank() inside the tile and
// meta_group_rank() (which tile). Then a tree reduction inside one tile of 8 values, as
// cg::reduce does with shuffles: at each step lane i adds the value of lane i + offset.
#include <cstdio>
#include <vector>

int main()
{
    const int blockSize = 64;
    const int tileSize = 16;
    std::printf("block of %d threads, tiles of %d: %d tiles\n", blockSize, tileSize, blockSize / tileSize);
    for (int t = 0; t < blockSize; t += 13) {
        std::printf("  block thread %2d -> tile %d (meta_group_rank), rank %2d in tile (thread_rank)\n", t,
                    t / tileSize, t % tileSize);
    }
    std::vector<int> v = {3, 1, 4, 1, 5, 9, 2, 6};
    const int n = static_cast<int>(v.size());
    std::printf("tile of %d lanes, values:", n);
    for (int x : v) {
        std::printf(" %d", x);
    }
    std::printf("\n");
    for (int offset = n / 2; offset > 0; offset /= 2) {
        std::vector<int> next = v;
        for (int lane = 0; lane < n; ++lane) {
            const int src = lane + offset;                 // shfl_down: read lane + offset
            next[static_cast<std::size_t>(lane)] += (src < n) ? v[static_cast<std::size_t>(src)] : 0;
        }
        v = next;
        std::printf("  offset %d:", offset);
        for (int x : v) {
            std::printf(" %d", x);
        }
        std::printf("\n");
    }
    std::printf("lane 0 holds the tile sum: %d\n", v[0]);
    return 0;
}
