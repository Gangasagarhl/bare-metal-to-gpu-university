// F6-15 Listing 1: a warp simulator for shuffles and votes, for warps of 32 or 64 lanes.
// The lane-selection rules are copied from the HIP 5.7 header read in this build
// (amd_warp_functions.h: __shfl, __shfl_up, __shfl_down, __shfl_xor); the chapter explains
// how they compare with the CUDA rules. One call = one warp-wide instruction.
#include <bit>
#include <cstdint>
#include <cstdio>
#include <vector>

using Lanes = std::vector<long>;

Lanes shflIdx(const Lanes& v, int src, int width)
{
    const int w = static_cast<int>(v.size());
    Lanes r(w);
    for (int self = 0; self < w; ++self) {
        int index = (src & (width - 1)) + (self & ~(width - 1));
        r[self] = v[index];
    }
    return r;
}

Lanes shflUp(const Lanes& v, int delta, int width)
{
    const int w = static_cast<int>(v.size());
    Lanes r(w);
    for (int self = 0; self < w; ++self) {
        int index = self - delta;
        index = (index < (self & ~(width - 1))) ? self : index;      // below the segment: keep own
        r[self] = v[index];
    }
    return r;
}

Lanes shflDown(const Lanes& v, int delta, int width)
{
    const int w = static_cast<int>(v.size());
    Lanes r(w);
    for (int self = 0; self < w; ++self) {
        int index = self + delta;
        index = ((self & (width - 1)) + delta >= width) ? self : index; // past the segment: keep own
        r[self] = v[index];
    }
    return r;
}

Lanes shflXor(const Lanes& v, int mask, int width)
{
    const int w = static_cast<int>(v.size());
    Lanes r(w);
    for (int self = 0; self < w; ++self) {
        int index = self ^ mask;
        index = (index >= ((self + width) & ~(width - 1))) ? self : index;
        r[self] = v[index];
    }
    return r;
}

std::uint64_t ballot(const std::vector<bool>& pred)
{
    std::uint64_t m = 0;
    for (std::size_t lane = 0; lane < pred.size(); ++lane) {
        if (pred[lane]) { m |= (std::uint64_t{1} << lane); }
    }
    return m;
}

void show(const char* label, const Lanes& v)
{
    std::printf("%-26s", label);
    for (int lane = 0; lane < 8; ++lane) { std::printf(" %4ld", v[lane]); }
    std::printf("  ... lane %zu: %ld\n", v.size() - 1, v.back());
}

Lanes iota(int w, long first)
{
    Lanes v(w);
    for (int i = 0; i < w; ++i) { v[i] = first + i; }
    return v;
}

int main()
{
    const int W = 32;
    std::printf("== 1. sum with shfl_down, 32 lanes holding 1..32 (lanes 0-7 shown)\n");
    Lanes v = iota(W, 1);
    show("start", v);
    for (int off = W / 2; off > 0; off /= 2) {
        Lanes got = shflDown(v, off, W);
        for (int i = 0; i < W; ++i) { v[i] += got[i]; }
        char label[32];
        std::snprintf(label, sizeof label, "after offset %d", off);
        show(label, v);
    }
    std::printf("lane 0 holds %ld (expected 32*33/2 = %d)\n\n", v[0], 32 * 33 / 2);

    std::printf("== 2. butterfly with shfl_xor: every lane ends with the total\n");
    v = iota(W, 1);
    for (int m = W / 2; m > 0; m /= 2) {
        Lanes got = shflXor(v, m, W);
        for (int i = 0; i < W; ++i) { v[i] += got[i]; }
    }
    show("after 5 xor steps", v);
    std::printf("\n== 3. inclusive scan with shfl_up, every lane holding 1\n");
    v = Lanes(W, 1);
    for (int d = 1; d < W; d *= 2) {
        Lanes got = shflUp(v, d, W);
        for (int i = 0; i < W; ++i) {
            if ((i & (W - 1)) >= d) { v[i] += got[i]; }               // lanes below d keep their value
        }
    }
    show("after 5 up steps", v);
    std::printf("\n== 4. ballot and popc: keep the lanes whose value is a multiple of 3\n");
    std::vector<bool> keep(W);
    v = iota(W, 100);
    for (int i = 0; i < W; ++i) { keep[i] = v[i] % 3 == 0; }
    std::uint64_t mask = ballot(keep);
    std::printf("ballot mask 0x%08llx, popc %d\n", static_cast<unsigned long long>(mask), std::popcount(mask));
    std::printf("compacted:");
    for (int i = 0; i < W; ++i) {
        if (keep[i]) {
            int slot = std::popcount(mask & ((std::uint64_t{1} << i) - 1));   // kept lanes below me
            std::printf(" [%d]=%ld", slot, v[i]);
        }
    }
    std::printf("\n\n== 5. width 16: two independent sums of 16 lanes in one 32-lane warp\n");
    v = iota(W, 1);
    for (int off = 8; off > 0; off /= 2) {
        Lanes got = shflDown(v, off, 16);
        for (int i = 0; i < W; ++i) { v[i] += got[i]; }
    }
    std::printf("lane 0: %ld (1..16), lane 16: %ld (17..32)\n\n", v[0], v[16]);

    std::printf("== 6. the same code on a 64-lane wavefront, every lane holding 1\n");
    const int W64 = 64;
    v = Lanes(W64, 1);
    for (int off = 16; off > 0; off /= 2) {                          // hard-coded for 32 lanes
        Lanes got = shflDown(v, off, W64);
        for (int i = 0; i < W64; ++i) { v[i] += got[i]; }
    }
    std::printf("loop starting at offset 16:        lane 0 = %ld (expected 64)\n", v[0]);
    v = Lanes(W64, 1);
    for (int off = W64 / 2; off > 0; off /= 2) {                      // derived from the width
        Lanes got = shflDown(v, off, W64);
        for (int i = 0; i < W64; ++i) { v[i] += got[i]; }
    }
    std::printf("loop starting at offset width / 2: lane 0 = %ld (expected 64)\n", v[0]);
    std::vector<bool> all(W64, true);
    std::uint64_t m64 = ballot(all);
    std::uint32_t m32 = static_cast<std::uint32_t>(m64);              // stored in a 32-bit variable
    std::printf("ballot of 64 true lanes: popc of 64-bit mask = %d, of 32-bit copy = %d\n",
                std::popcount(m64), std::popcount(m32));
    return 0;
}
