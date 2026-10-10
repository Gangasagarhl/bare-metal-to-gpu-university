// F1-68 Listing 3: a colour filter mosaic (RGGB pattern) and the simplest
// reconstruction: every 2 x 2 block of the mosaic becomes one colour pixel.
// The scene is 8 x 8 pixels: red on the left, white on the right, with the edge
// between columns 4 and 5 (in the middle of a block). Values are 0..9.
#include <array>
#include <cstdio>

int main()
{
    // scene[y][x] = {R, G, B}
    std::array<std::array<std::array<int, 3>, 8>, 8> scene{};
    for (int y = 0; y < 8; ++y) {
        for (int x = 0; x < 8; ++x) {
            scene[y][x] = x < 5 ? std::array<int, 3>{9, 0, 0} : std::array<int, 3>{9, 9, 9};
        }
    }
    // Mosaic: each sensor pixel sees ONE colour. Row even: R G R G ...; row odd: G B G B ...
    std::printf("mosaic (filter letter + value the pixel measured):\n");
    std::array<std::array<int, 8>, 8> raw{};
    for (int y = 0; y < 8; ++y) {
        std::printf("  ");
        for (int x = 0; x < 8; ++x) {
            const int ch = (y % 2 == 0) ? (x % 2 == 0 ? 0 : 1) : (x % 2 == 0 ? 1 : 2);
            raw[y][x] = scene[y][x][ch];
            std::printf("%c%d ", "RGB"[ch], raw[y][x]);
        }
        std::printf("\n");
    }
    std::printf("\n2 x 2 block reconstruction (R, G = mean of the two greens, B):\n  ");
    for (int bx = 0; bx < 4; ++bx) {
        const int r = raw[0][2 * bx];
        const double g = (raw[0][2 * bx + 1] + raw[1][2 * bx]) / 2.0;
        const int b = raw[1][2 * bx + 1];
        std::printf("(%d,%.1f,%d) ", r, g, b);
    }
    std::printf("\ntrue colours of the same blocks:\n  ");
    for (int bx = 0; bx < 4; ++bx) {
        std::printf("cols %d-%d: %s  ", 2 * bx, 2 * bx + 1,
                    2 * bx + 1 < 5 ? "red" : (2 * bx >= 5 ? "white" : "half red, half white"));
    }
    std::printf("\n");
    return 0;
}
