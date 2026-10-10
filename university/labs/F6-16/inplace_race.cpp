// F6-16 forensic evidence: the Hillis-Steele scan written IN PLACE (no second buffer), replayed
// for one block of 256 threads = 8 warps. Within one warp, all lanes read before any lane writes
// (one instruction at a time). Between warps there is no order: each step, the warps run in an
// order chosen by a seeded scheduler. The block has __syncthreads() between steps, as in the
// scenario's kernel; what is missing is the second buffer.
#include <algorithm>
#include <cstdio>
#include <numeric>
#include <random>
#include <vector>

constexpr int N = 256;

// mode 0: warps in order 0..7; mode 1: in order 7..0; mode 2: shuffled with the seed
int wrongOutputs(int mode, unsigned seed, int& firstWrong)
{
    std::vector<long> a(N, 1);                               // input: all ones, answer: 1, 2, ..., 256
    std::mt19937 rng(seed);
    for (int d = 1; d < N; d *= 2) {
        std::vector<int> order(N / 32);
        std::iota(order.begin(), order.end(), 0);
        if (mode == 1) { std::reverse(order.begin(), order.end()); }
        if (mode == 2) { std::shuffle(order.begin(), order.end(), rng); }
        for (int w : order) {
            long got[32];
            for (int lane = 0; lane < 32; ++lane) {          // the warp's read instruction
                int i = w * 32 + lane;
                got[lane] = (i >= d) ? a[i - d] : 0;
            }
            for (int lane = 0; lane < 32; ++lane) { a[w * 32 + lane] += got[lane]; }   // its write
        }
        // __syncthreads() here: the next step starts after every warp finished this one
    }
    int wrong = 0;
    firstWrong = -1;
    for (int i = 0; i < N; ++i) {
        if (a[i] != i + 1) { ++wrong; if (firstWrong < 0) { firstWrong = i; } }
    }
    return wrong;
}

int main()
{
    int first = 0;
    std::printf("%-34s %-14s %s\n", "warp order in every step", "wrong of 256", "first wrong index");
    int w = wrongOutputs(0, 0, first);
    std::printf("%-34s %-14d %d\n", "warp 0 first, then 1, 2, ...", w, first);
    w = wrongOutputs(1, 0, first);
    std::printf("%-34s %-14d %s\n", "warp 7 first, then 6, 5, ...", w, first < 0 ? "none" : "?");
    for (unsigned s = 1; s <= 4; ++s) {
        w = wrongOutputs(2, s, first);
        char label[40];
        std::snprintf(label, sizeof label, "shuffled (seed %u)", s);
        std::printf("%-34s %-14d %d\n", label, w, first);
    }
    return 0;
}
