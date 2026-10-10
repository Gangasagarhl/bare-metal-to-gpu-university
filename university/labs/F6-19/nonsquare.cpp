// F6-19 forensic evidence: CPU replay of the scenario's tiled transpose, whose second loop
// computes the output index with the input's width instead of its height:
//     out[(y + j) * width + x]        (bug)     instead of     out[(y + j) * height + x]
// The replay counts wrong elements, elements never written, and writes that would land outside
// the output buffer (on a GPU those corrupt other memory; Compute Sanitizer reports them).
#include <cstdio>
#include <vector>

constexpr int TILE = 32, ROWS = 8;

void replay(int w, int h, bool bug)
{
    const long size = static_cast<long>(w) * h;
    std::vector<float> in(size), out(size, -1.0f);
    for (long k = 0; k < size; ++k) { in[k] = static_cast<float>(k); }
    long outside = 0;
    std::vector<float> tile(TILE * (TILE + 1));
    for (int by = 0; by < (h + TILE - 1) / TILE; ++by) {
        for (int bx = 0; bx < (w + TILE - 1) / TILE; ++bx) {
            for (int ty = 0; ty < ROWS; ++ty) {
                for (int tx = 0; tx < TILE; ++tx) {
                    int x = bx * TILE + tx, y = by * TILE + ty;
                    for (int j = 0; j < TILE; j += ROWS) {
                        if (x < w && y + j < h) { tile[(ty + j) * (TILE + 1) + tx] = in[static_cast<long>(y + j) * w + x]; }
                    }
                }
            }
            for (int ty = 0; ty < ROWS; ++ty) {
                for (int tx = 0; tx < TILE; ++tx) {
                    int x = by * TILE + tx, y = bx * TILE + ty;
                    for (int j = 0; j < TILE; j += ROWS) {
                        if (x < h && y + j < w) {
                            long dst = static_cast<long>(y + j) * (bug ? w : h) + x;
                            if (dst >= size) { ++outside; continue; }
                            out[dst] = tile[tx * (TILE + 1) + ty + j];
                        }
                    }
                }
            }
        }
    }
    long wrong = 0, unwritten = 0;
    for (int r = 0; r < h; ++r) {
        for (int c = 0; c < w; ++c) {
            float got = out[static_cast<long>(c) * h + r];
            unwritten += (got == -1.0f);
            wrong += (got != in[static_cast<long>(r) * w + c]);
        }
    }
    std::printf("%-5s %5d x %-5d wrong %-8ld never written %-8ld writes outside the buffer %ld\n",
                bug ? "bug" : "fixed", w, h, wrong, unwritten, outside);
}

int main()
{
    for (bool bug : {true, false}) {
        replay(64, 64, bug);
        replay(1024, 1024, bug);
        replay(1000, 700, bug);
        replay(700, 1000, bug);
    }
    return 0;
}
