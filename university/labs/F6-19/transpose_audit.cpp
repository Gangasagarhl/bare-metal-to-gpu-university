// F6-19 Listing 2: an audit of the five kernels of Listing 1, without a GPU.
// Part 1: for one warp (threadIdx.y = 0, threadIdx.x = 0..31) and j = 0, how many 32-byte global
//         memory segments each global access touches (model: 32-byte segments), and how many
//         passes each shared-memory access needs (model: 32 banks x 4 bytes).
// Part 2: replays every block of every kernel on the CPU for awkward shapes and checks the result.
#include <algorithm>
#include <cstdio>
#include <map>
#include <set>
#include <vector>

constexpr int TILE = 32, ROWS = 8;

int segments(const std::vector<long>& byteAddr)
{
    std::set<long> s;
    for (long a : byteAddr) { s.insert(a / 32); }
    return static_cast<int>(s.size());
}

int passes(const std::vector<int>& words)
{
    std::map<int, std::set<int>> perBank;
    for (int w : words) { perBank[w % 32].insert(w); }
    std::size_t worst = 0;
    for (const auto& kv : perBank) { worst = std::max(worst, kv.second.size()); }
    return static_cast<int>(worst);
}

// shared word index for MODE (0 pitch 32, 1 pitch 33, 2 swizzle) of tile[r][c]
int word(int mode, int r, int c)
{
    if (mode == 1) { return r * 33 + c; }
    if (mode == 2) { return r * 32 + (c ^ r); }
    return r * 32 + c;
}

// CPU replay of one kernel: version 0 copy, 1 naive, 2-4 tiled modes 0-2
std::vector<float> replay(int version, const std::vector<float>& in, int w, int h)
{
    std::vector<float> out(in.size(), -1.0f);
    std::vector<float> tile(TILE * 33);
    for (int by = 0; by < (h + TILE - 1) / TILE; ++by) {
        for (int bx = 0; bx < (w + TILE - 1) / TILE; ++bx) {
            int mode = version - 2;
            for (int ty = 0; ty < ROWS; ++ty) {
                for (int tx = 0; tx < TILE; ++tx) {
                    int x = bx * TILE + tx, y = by * TILE + ty;
                    for (int j = 0; j < TILE; j += ROWS) {
                        if (!(x < w && y + j < h)) { continue; }
                        float v = in[static_cast<std::size_t>(y + j) * w + x];
                        if (version == 0) { out[static_cast<std::size_t>(y + j) * w + x] = v; }
                        else if (version == 1) { out[static_cast<std::size_t>(x) * h + y + j] = v; }
                        else {
                            int r = ty + j;
                            int c = (mode == 2) ? (tx ^ r) : tx;
                            tile[mode == 1 ? r * 33 + c : r * 32 + c] = v;
                        }
                    }
                }
            }
            if (version < 2) { continue; }
            // __syncthreads()
            for (int ty = 0; ty < ROWS; ++ty) {
                for (int tx = 0; tx < TILE; ++tx) {
                    int x = by * TILE + tx, y = bx * TILE + ty;
                    for (int j = 0; j < TILE; j += ROWS) {
                        int r = tx;
                        int c = (mode == 2) ? ((ty + j) ^ r) : (ty + j);
                        if (x < h && y + j < w) {
                            out[static_cast<std::size_t>(y + j) * h + x] = tile[mode == 1 ? r * 33 + c : r * 32 + c];
                        }
                    }
                }
            }
        }
    }
    return out;
}

int main()
{
    const int w = 4096, h = 4096;
    std::printf("Part 1: one warp, j = 0, 4-byte floats, %d x %d matrix\n", w, h);
    std::printf("%-13s %-12s %-12s %-14s %s\n", "kernel", "load segs", "store segs", "smem write", "smem read");
    const char* names[] = {"copyTile", "naive", "tiledNoPad", "tiledPad", "tiledSwizzle"};
    for (int v = 0; v < 5; ++v) {
        std::vector<long> ld, st;
        std::vector<int> sw, sr;
        for (int tx = 0; tx < 32; ++tx) {
            int ty = 0, j = 0, x = tx, y = ty;                  // block (0, 0)
            ld.push_back(4L * ((y + j) * static_cast<long>(w) + x));
            if (v == 0) { st.push_back(4L * ((y + j) * static_cast<long>(w) + x)); }
            else if (v == 1) { st.push_back(4L * (x * static_cast<long>(h) + y + j)); }
            else {
                int mode = v - 2;
                sw.push_back(word(mode, ty + j, tx));
                sr.push_back(word(mode, tx, ty + j));
                st.push_back(4L * ((y + j) * static_cast<long>(h) + x));
            }
        }
        std::printf("%-13s %-12d %-12d", names[v], segments(ld), segments(st));
        if (v < 2) { std::printf(" %-14s %s\n", "-", "-"); }
        else { std::printf(" %-14d %d\n", passes(sw), passes(sr)); }
    }
    std::printf("\nPart 2: CPU replay of every block, compared with the true transpose\n");
    const int shapes[][2] = {{1000, 700}, {33, 65}, {1, 77}, {64, 64}};
    for (const auto& s : shapes) {
        const int sw = s[0], sh = s[1];
        std::vector<float> in(static_cast<std::size_t>(sw) * sh);
        for (std::size_t k = 0; k < in.size(); ++k) { in[k] = static_cast<float>(k); }
        std::printf("%4d x %-4d", sw, sh);
        for (int v = 1; v < 5; ++v) {
            std::vector<float> out = replay(v, in, sw, sh);
            long errors = 0;
            for (int r = 0; r < sh; ++r) {
                for (int c = 0; c < sw; ++c) {
                    errors += out[static_cast<std::size_t>(c) * sh + r] != in[static_cast<std::size_t>(r) * sw + c];
                }
            }
            std::printf("  %s: %ld errors", names[v], errors);
        }
        std::printf("\n");
    }
    return 0;
}
