// F6-27 Listing 1: a tiny layout algebra in the spirit of CuTe, written from scratch.
// A layout is a function from a coordinate to an index: index = sum(coord[d] * stride[d]).
// Shapes and strides may be nested one level ((rows-in-tile, tiles), ...), which is how
// one layout describes a tiled matrix. This is the university's own code, not CuTe's API.
#include <cstdio>
#include <string>
#include <vector>

struct Mode                         // one dimension: a shape and a stride, possibly nested
{
    std::vector<int> shape;         // e.g. {4} or {2, 4} (inner extent first)
    std::vector<int> stride;
    int size() const
    {
        int s = 1;
        for (int x : shape) {
            s *= x;
        }
        return s;
    }
    int map(int c) const            // a flat coordinate inside this mode -> offset
    {
        int off = 0;
        for (size_t i = 0; i < shape.size(); ++i) {
            off += (c % shape[i]) * stride[i];
            c /= shape[i];
        }
        return off;
    }
};

struct Layout2D
{
    Mode rows, cols;
    int operator()(int r, int c) const
    {
        return rows.map(r) + cols.map(c);
    }
};

void show(const char* title, const Layout2D& L)
{
    std::printf("%s\n", title);
    for (int r = 0; r < L.rows.size(); ++r) {
        for (int c = 0; c < L.cols.size(); ++c) {
            std::printf("%4d", L(r, c));
        }
        std::printf("\n");
    }
}

int main()
{
    // 1. The same 4 x 8 matrix, two layouts.
    show("(1a) row-major 4 x 8: shape (4, 8), stride (8, 1)", {{{4}, {8}}, {{8}, {1}}});
    show("(1b) column-major 4 x 8: shape (4, 8), stride (1, 4)", {{{4}, {1}}, {{8}, {4}}});

    // 2. An 8 x 8 row-major matrix stored as 2 x 2 tiles of 4 x 4, tile after tile:
    //    row r = (r % 4, r / 4) has stride (4, 32); column c = (c % 4, c / 4) has stride (1, 16).
    show("(2) 8 x 8 stored tile by tile (4 x 4 tiles, row-major inside and between tiles):\n"
         "    shape ((4, 2), (4, 2)), stride ((4, 32), (1, 16))",
         {{{4, 2}, {4, 32}}, {{4, 2}, {1, 16}}});

    // 3. Thread-value partition: 32 threads own an 8 x 16 tile, each thread 4 values.
    //    thread t -> (row t / 4, columns 4 * (t % 4) + v), so a thread's 4 values are contiguous.
    std::printf("(3) owner thread of each element of an 8 x 16 tile (thread layout (8, 4):(4, 1), "
                "4 values each along a row)\n");
    for (int r = 0; r < 8; ++r) {
        for (int c = 0; c < 16; ++c) {
            std::printf("%3d", r * 4 + c / 4);
        }
        std::printf("\n");
    }

    // 4. Swizzling: reading one column of a 32 x 32 float tile in shared memory, one thread
    //    per row. Model: 32 banks, bank = (word address) % 32 (the F6-11 model; check the
    //    vendor guide for your GPU). Count how many threads hit the busiest bank.
    auto worst = [](auto addr) {
        int most = 0;
        for (int col = 0; col < 32; ++col) {
            std::vector<int> hits(32, 0);
            for (int row = 0; row < 32; ++row) {
                ++hits[size_t(addr(row, col) % 32)];
            }
            for (int h : hits) {
                most = h > most ? h : most;
            }
        }
        return most;
    };
    std::printf("(4) column read of a 32 x 32 float tile: threads on the busiest bank\n");
    std::printf("    plain       addr = row*32 + col              : %d\n",
                worst([](int r, int c) { return r * 32 + c; }));
    std::printf("    padded      addr = row*33 + col              : %d\n",
                worst([](int r, int c) { return r * 33 + c; }));
    std::printf("    swizzled    addr = row*32 + (col XOR row)     : %d\n",
                worst([](int r, int c) { return r * 32 + (c ^ r); }));
    // a swizzle is a permutation inside each row: nothing is lost and no space is added
    bool permutation = true;
    for (int r = 0; r < 32; ++r) {
        std::vector<int> seen(32, 0);
        for (int c = 0; c < 32; ++c) {
            ++seen[size_t(c ^ r)];
        }
        for (int s : seen) {
            permutation = permutation && s == 1;
        }
    }
    std::printf("    swizzle is a permutation of every row: %s; extra words: plain 0, padded 32, "
                "swizzled 0\n", permutation ? "yes" : "no");
    return 0;
}
