// ref_triangle.cpp - DR405 F4-47: the CPU reference renderer for milestone G6's acceptance
// test ("matches a CPU-rendered reference of the same triangle within the documented
// rasterisation rules"). It samples pixel centres with edge functions in integer
// arithmetic (coordinates in 1/16 pixel) and applies a top-left fill rule, so two
// triangles that share an edge cover every pixel of the edge exactly once. A second
// renderer with an inclusive rule (">= 0" on every edge) is compared with it.
// Which rule the VideoCore IV hardware applies is NOT verified here: check the Broadcom
// "VideoCore IV 3D Architecture Reference Guide" before using this as the oracle.
#include <array>
#include <cstdint>
#include <cstdio>
#include <vector>

namespace {
constexpr int W = 24, H = 12, SUB = 16;               // image size; subpixel steps per pixel
struct P { int64_t x, y; };                            // in 1/16 pixel

int64_t edge(P a, P b, P p) { return (b.x - a.x) * (p.y - a.y) - (b.y - a.y) * (p.x - a.x); }

// Top-left rule for a clockwise triangle in y-down screen space (edge(a, b, c) > 0):
// a "top" edge is horizontal with the interior below it; a "left" edge goes upwards.
bool top_left(P a, P b) { return (a.y == b.y && b.x > a.x) || (b.y < a.y); }

void raster(std::vector<int>& count, P v0, P v1, P v2, bool inclusive)
{
    if (edge(v0, v1, v2) < 0) std::swap(v1, v2);       // make every triangle clockwise
    const int64_t b0 = inclusive || top_left(v1, v2) ? 0 : -1, b1 = inclusive || top_left(v2, v0) ? 0 : -1,
                  b2 = inclusive || top_left(v0, v1) ? 0 : -1;
    for (int y = 0; y < H; ++y)
        for (int x = 0; x < W; ++x) {
            const P c{x * SUB + SUB / 2, y * SUB + SUB / 2};   // the pixel centre
            const int64_t w0 = edge(v1, v2, c), w1 = edge(v2, v0, c), w2 = edge(v0, v1, c);
            if (w0 + b0 >= 0 && w1 + b1 >= 0 && w2 + b2 >= 0) ++count[y * W + x];
        }
}

void show(const char* title, const std::vector<int>& c)
{
    int once = 0, twice = 0, none = 0;
    std::printf("%s\n", title);
    for (int y = 0; y < H; ++y) {
        std::printf("  ");
        for (int x = 0; x < W; ++x) {
            const int n = c[y * W + x];
            std::putchar(n == 0 ? '.' : n == 1 ? '#' : '2');
            (n == 0 ? none : n == 1 ? once : twice) += 1;
        }
        std::putchar('\n');
    }
    std::printf("  pixels covered once %d, twice %d, not at all %d\n\n", once, twice, none);
}
}  // namespace

int main()
{
    // A square from (4.5,2.5) to (12.5,10.5), split along its diagonal into two triangles.
    // Its edges pass exactly through pixel centres, the hard case for any fill rule:
    // every pixel should be drawn exactly once, and the square should cover 8 x 8 pixels.
    const P a{4 * SUB + 8, 2 * SUB + 8}, b{12 * SUB + 8, 2 * SUB + 8}, c{12 * SUB + 8, 10 * SUB + 8},
        d{4 * SUB + 8, 10 * SUB + 8};
    std::vector<int> ref(W * H, 0), inc(W * H, 0);
    raster(ref, a, b, c, false);
    raster(ref, a, c, d, false);
    raster(inc, a, b, c, true);
    raster(inc, a, c, d, true);
    show("reference (top-left rule): two triangles sharing the diagonal a-c; # once, 2 twice", ref);
    show("inclusive rule (>= 0 on every edge): the same two triangles", inc);
    int diff = 0;
    for (int i = 0; i < W * H; ++i) diff += ref[i] != inc[i];
    std::printf("pixels that differ between the two renderers: %d\n", diff);
    // the single-triangle acceptance image: count and checksum of the reference coverage
    std::vector<int> one(W * H, 0);
    raster(one, P{6 * SUB + 3, 1 * SUB}, P{21 * SUB, 7 * SUB + 8}, P{3 * SUB, 11 * SUB}, false);
    uint32_t h = 2166136261u, n = 0;
    for (int v : one) {
        h = (h ^ static_cast<uint32_t>(v)) * 16777619u;
        n += static_cast<uint32_t>(v);
    }
    show("one triangle (6.1875,1) (21,7.5) (3,11): the G6 acceptance reference", one);
    std::printf("reference coverage: %u pixels, fnv1a 0x%08x\n", n, h);
    return 0;
}
