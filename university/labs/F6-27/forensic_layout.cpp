// F6-27 forensic evidence: three candidate layouts for "an 8 x 8 matrix stored as 2 x 2
// tiles of 4 x 4" from a design review, checked by brute force: does every coordinate get
// its own index in 0..63 (a bijection)? Same layout rule as Listing 1:
// index = sum over sub-modes of (coordinate part) * stride.
#include <cstdio>
#include <vector>

struct Candidate
{
    const char* name;
    int rowStride[2];      // row r = (r % 4, r / 4)
    int colStride[2];      // col c = (c % 4, c / 4)
};

int main()
{
    const Candidate cands[] = {
        {"A ((4,32),(1,16))", {4, 32}, {1, 16}},
        {"B ((4,16),(1,32))", {4, 16}, {1, 32}},
        {"C ((8,32),(1,16))", {8, 32}, {1, 16}},
    };
    for (const Candidate& k : cands) {
        std::vector<int> hits(128, 0);
        int maxIndex = 0;
        for (int r = 0; r < 8; ++r) {
            for (int c = 0; c < 8; ++c) {
                const int idx = (r % 4) * k.rowStride[0] + (r / 4) * k.rowStride[1] +
                                (c % 4) * k.colStride[0] + (c / 4) * k.colStride[1];
                maxIndex = idx > maxIndex ? idx : maxIndex;
                if (idx < 128) {
                    ++hits[size_t(idx)];
                }
            }
        }
        int twice = 0, unused = 0;
        for (int i = 0; i < 64; ++i) {
            twice += hits[size_t(i)] > 1 ? 1 : 0;
            unused += hits[size_t(i)] == 0 ? 1 : 0;
        }
        std::printf("%-18s largest index %3d; indices 0..63 used twice or more: %2d, never used: %2d"
                    "; %s\n", k.name, maxIndex, twice, unused,
                    (twice == 0 && unused == 0 && maxIndex == 63) ? "bijection onto 0..63"
                                                                   : "NOT a bijection onto 0..63");
    }
    return 0;
}
