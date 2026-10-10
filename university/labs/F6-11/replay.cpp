// F6-11 forensic evidence: replays the shared-memory addresses of the transpose kernels in
// transpose_bank.cu for one warp (threadIdx.y fixed, threadIdx.x = 0..31) and prints, per
// shared-memory instruction, how many passes the bank model needs. This is the university's
// model, NOT Nsight Compute output (the build container has no GPU).
#include <algorithm>
#include <cstdio>
#include <map>
#include <set>

constexpr int BANKS = 32;
constexpr int TILE = 32;
constexpr int ROWS = 8;

int passesForWarp(int pitch, int ty, int j, bool columnRead)
{
    std::map<int, std::set<int>> wordsInBank;
    for (int tx = 0; tx < 32; ++tx) {
        int row = columnRead ? tx : ty + j;      // tile[threadIdx.x][threadIdx.y + j]  (read)
        int col = columnRead ? ty + j : tx;      // tile[threadIdx.y + j][threadIdx.x]  (write)
        int word = row * pitch + col;
        wordsInBank[word % BANKS].insert(word);
    }
    std::size_t worst = 0;
    for (const auto& kv : wordsInBank) { worst = std::max(worst, kv.second.size()); }
    return static_cast<int>(worst);
}

int main()
{
    std::printf("replay of one warp (threadIdx.y = 0), model: %d banks x 4 bytes\n", BANKS);
    for (int pitch : {TILE, TILE + 1}) {
        std::printf("\nkernel %s (tile pitch %d words)\n", pitch == TILE ? "transposeNoPad" : "transposePad", pitch);
        std::printf("  %-34s %-6s %s\n", "instruction", "j", "passes per warp");
        int requests = 0, total = 0;
        for (int j = 0; j < TILE; j += ROWS) {
            int p = passesForWarp(pitch, 0, j, false);
            std::printf("  %-34s %-6d %d\n", "store tile[ty+j][tx]  (row)", j, p);
            requests += 1; total += p;
        }
        for (int j = 0; j < TILE; j += ROWS) {
            int p = passesForWarp(pitch, 0, j, true);
            std::printf("  %-34s %-6d %d\n", "load  tile[tx][ty+j]  (column)", j, p);
            requests += 1; total += p;
        }
        std::printf("  per warp: %d shared-memory requests, %d passes, %d extra passes caused by conflicts\n",
                    requests, total, total - requests);
        long warps = 4096L / TILE * (4096L / TILE) * (TILE * ROWS / 32);
        std::printf("  whole 4096 x 4096 transpose (%ld warps): %ld extra passes\n", warps, warps * (total - requests));
    }
    return 0;
}
