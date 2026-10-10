// lanes_sim.cpp - the university's own lock-step model of waveSum (wave_sum.hip) on the CPU.
// It runs one block of 64 threads as wavefronts of W lanes (W = 32 or 64) and applies
// __shfl_xor exactly as HIP 5.7's AMD header computes the source lane:
//     index = self ^ lane_mask;
//     index = index >= ((self + width) & ~(width - 1)) ? self : index;
// It is a model for reading per-lane evidence, not a GPU: no timing, no memory system.
#include <cstdio>
#include <vector>

// one butterfly step for a whole wavefront: every lane reads the OLD value of its partner
static std::vector<int> shflXorStep(const std::vector<int>& v, int laneMask, int width)
{
    std::vector<int> got(v.size());
    for (int self = 0; self < static_cast<int>(v.size()); ++self) {
        int index = self ^ laneMask;
        index = index >= ((self + width) & ~(width - 1)) ? self : index;
        got[self] = v[index];
    }
    return got;
}

// waveSum for one block; firstOffset 16 is the ported code, W / 2 the fix
static int runBlock(int blockSize, int w, int firstOffset, std::vector<int>& lanesOut)
{
    int total = 0;
    for (int base = 0; base < blockSize; base += w) {         // one wavefront at a time
        std::vector<int> v(w);
        for (int lane = 0; lane < w; ++lane) {
            v[lane] = base + lane + 1;                        // in[i] = i + 1
        }
        for (int offset = firstOffset; offset > 0; offset /= 2) {
            const std::vector<int> partner = shflXorStep(v, offset, w);
            for (int lane = 0; lane < w; ++lane) {
                v[lane] += partner[lane];
            }
        }
        for (int lane = 0; lane < w; ++lane) {
            lanesOut[base + lane] = v[lane];
        }
        total += v[0];                                        // leader: threadIdx.x % W == 0
    }
    return total;
}

static void report(const char* title, int w, int firstOffset)
{
    const int blockSize = 64;
    std::vector<int> lanes(blockSize);
    const int total = runBlock(blockSize, w, firstOffset, lanes);
    std::printf("%s (W = %d lanes, loop starts at offset %d)\n", title, w, firstOffset);
    std::printf("lane:value after the shuffle loop\n");
    for (int i = 0; i < blockSize; ++i) {
        std::printf("%2d:%5d%s", i, lanes[i], (i % 8 == 7) ? "\n" : "   ");
    }
    std::printf("total = %d (expected %d)\n\n", total, blockSize * (blockSize + 1) / 2);
}

int main()
{
    report("ported code, 32-lane warps", 32, 16);
    report("ported code, 64-lane wavefronts", 64, 16);
    report("fixed code, 32-lane warps", 32, 32 / 2);
    report("fixed code, 64-lane wavefronts", 64, 64 / 2);
    return 0;
}
