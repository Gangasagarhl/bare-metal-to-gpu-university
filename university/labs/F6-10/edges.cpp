// F6-10 forensic evidence: a CPU emulation of the stencil block algorithm, run twice:
//   good : the first RADIUS threads also load the left and right halo (as in Listing 1)
//   buggy: nobody loads the halo (the version in the forensic scenario)
// The emulator keeps one tile array for all blocks, so halo cells that nobody writes still hold what
// the previous block left there. On a GPU their content is simply undefined; this choice only makes
// the emulation repeatable.
#include <cstdio>
#include <map>
#include <vector>

constexpr int RADIUS = 3;
constexpr int BLOCK = 256;

std::vector<int> emulate(const std::vector<int>& in, bool loadHalo)
{
    const int n = static_cast<int>(in.size());
    std::vector<int> out(in.size(), 0);
    std::vector<int> tile(BLOCK + 2 * RADIUS, 0);           // "shared memory", reused block after block
    const int blocks = (n + BLOCK - 1) / BLOCK;
    for (int b = 0; b < blocks; ++b) {
        for (int t = 0; t < BLOCK; ++t) {                    // phase 1: every thread loads
            int g = b * BLOCK + t;
            int s = t + RADIUS;
            tile[s] = (g < n) ? in[g] : 0;
            if (loadHalo && t < RADIUS) {
                int left = g - RADIUS;
                int right = g + BLOCK;
                tile[s - RADIUS] = (left >= 0 && left < n) ? in[left] : 0;
                tile[s + BLOCK] = (right < n) ? in[right] : 0;
            }
        }
        // __syncthreads(): phase 2 starts only after phase 1 finished for every thread
        for (int t = 0; t < BLOCK; ++t) {
            int g = b * BLOCK + t;
            if (g < n) {
                int sum = 0;
                for (int k = -RADIUS; k <= RADIUS; ++k) { sum += tile[t + RADIUS + k]; }
                out[g] = sum;
            }
        }
    }
    return out;
}

int main()
{
    const int n = 1000003;
    std::vector<int> in(n);
    for (int i = 0; i < n; ++i) { in[i] = (i * 7) % 11; }
    std::vector<int> ref(n, 0);
    for (int i = 0; i < n; ++i) {
        for (int k = -RADIUS; k <= RADIUS; ++k) {
            if (i + k >= 0 && i + k < n) { ref[i] += in[i + k]; }
        }
    }
    for (bool halo : {true, false}) {
        std::vector<int> out = emulate(in, halo);
        std::map<int, int> byThread;                         // wrong outputs per threadIdx.x
        int wrong = 0;
        for (int i = 0; i < n; ++i) {
            if (out[i] != ref[i]) { ++wrong; ++byThread[i % BLOCK]; }
        }
        std::printf("%s: %d wrong outputs of %d\n", halo ? "halo loaded    " : "halo NOT loaded", wrong, n);
        if (wrong > 0) {
            std::printf("  wrong outputs by threadIdx.x (thread: count):");
            for (const auto& kv : byThread) { std::printf(" %d:%d", kv.first, kv.second); }
            std::printf("\n  first wrong outputs (index: expected, got):");
            int shown = 0;
            for (int i = 0; i < n && shown < 6; ++i) {
                if (out[i] != ref[i]) { std::printf(" %d: %d, %d;", i, ref[i], out[i]); ++shown; }
            }
            std::printf("\n");
        }
    }
    return 0;
}
