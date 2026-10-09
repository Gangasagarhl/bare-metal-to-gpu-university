// refimg.cc - DR405 F4-43: the acceptance test "QEMU screen dump matches a reference image".
// Renders frame N with the same scene.h code the kernel uses and compares it, pixel by
// pixel, with a binary PPM (P6) screen dump written by QEMU's "screendump" command.
//   usage: refimg <frame number> <dump.ppm>
#include <cstdio>
#include <fstream>
#include <string>
#include <vector>
#include "scene.h"

int main(int argc, char** argv)
{
    if (argc != 3) {
        std::puts("usage: refimg <frame number> <dump.ppm>");
        return 2;
    }
    const int n = std::stoi(argv[1]);
    std::ifstream in(argv[2], std::ios::binary);
    std::string magic;
    int w = 0, h = 0, maxval = 0;
    in >> magic >> w >> h >> maxval;
    in.get();                                          // the single whitespace after maxval
    if (!in || magic != "P6" || maxval != 255 || w <= 0 || h <= 0) {
        std::printf("%s: not a P6 image with maxval 255\n", argv[2]);
        return 2;
    }
    std::vector<unsigned char> rgb(static_cast<size_t>(w) * h * 3);
    in.read(reinterpret_cast<char*>(rgb.data()), static_cast<std::streamsize>(rgb.size()));
    std::vector<uint32_t> ref(static_cast<size_t>(w) * h);
    scene::Fb fb{ref.data(), w, h};
    scene::frame(fb, n);
    long bad = 0;
    int x0 = w, y0 = h, x1 = -1, y1 = -1;
    for (int y = 0; y < h; ++y)
        for (int x = 0; x < w; ++x) {
            const size_t i = static_cast<size_t>(y) * w + x;
            const uint32_t got = (uint32_t{rgb[3 * i]} << 16) | (uint32_t{rgb[3 * i + 1]} << 8) | rgb[3 * i + 2];
            if (got != ref[i]) {
                ++bad;
                if (x < x0) x0 = x;
                if (y < y0) y0 = y;
                if (x > x1) x1 = x;
                if (y > y1) y1 = y;
            }
        }
    std::printf("frame %d: dump %dx%d, %ld of %ld pixels differ from the reference", n, w, h, bad,
                static_cast<long>(w) * h);
    if (bad) std::printf(" (inside x=%d..%d, y=%d..%d)", x0, x1, y0, y1);
    std::printf(": %s\n", bad ? "MISMATCH" : "MATCH");
    return bad ? 1 : 0;
}
