// F6-02 Listing 2: a CPU model of a 2-D launch (the university's own teaching model,
// not a GPU). It visits every (block, thread) pair the way a grid is defined, computes
// x and y with the same formulas as Listing 1, and draws which pixels were covered.
// Input: width height blockX blockY rounding(up|down) [askX askY]
#include <cstdio>
#include <iostream>
#include <string>
#include <vector>

int main()
{
    int width = 0, height = 0, bx = 0, by = 0;
    std::string rounding;
    int askX = -1, askY = -1;
    if (!(std::cin >> width >> height >> bx >> by >> rounding) || width <= 0 || height <= 0 ||
        bx <= 0 || by <= 0) {
        std::printf("invalid input\n");
        return 1;
    }
    std::cin >> askX >> askY;
    const bool up = rounding == "up";
    const int gx = up ? (width + bx - 1) / bx : width / bx;
    const int gy = up ? (height + by - 1) / by : height / by;
    std::printf("image %d x %d, block %d x %d, grid %d x %d (rounded %s)\n", width, height, bx,
                by, gx, gy, up ? "up" : "down");

    std::vector<int> owner(static_cast<std::size_t>(width) * height, -1);
    int launched = 0, idle = 0;
    for (int byi = 0; byi < gy; ++byi) {              // blockIdx.y
        for (int bxi = 0; bxi < gx; ++bxi) {          // blockIdx.x
            for (int ty = 0; ty < by; ++ty) {         // threadIdx.y
                for (int tx = 0; tx < bx; ++tx) {     // threadIdx.x
                    ++launched;
                    const int x = bxi * bx + tx;
                    const int y = byi * by + ty;
                    if (x < width && y < height) {
                        owner[static_cast<std::size_t>(y) * width + x] = byi * gx + bxi;
                        if (x == askX && y == askY) {
                            std::printf("pixel (%d,%d): block (%d,%d), thread (%d,%d), "
                                        "linear id in block %d, warp %d of its block\n",
                                        x, y, bxi, byi, tx, ty, ty * bx + tx, (ty * bx + tx) / 32);
                        }
                    } else {
                        ++idle;
                    }
                }
            }
        }
    }
    int missed = 0;
    for (int y = 0; y < height; ++y) {
        std::string row;
        for (int x = 0; x < width; ++x) {
            const int o = owner[static_cast<std::size_t>(y) * width + x];
            if (o < 0) { row += '.'; ++missed; } else { row += static_cast<char>('A' + o % 26); }
        }
        if (width <= 64 && height <= 32) {           // draw small images only
            std::printf("  %s\n", row.c_str());
        }
    }
    std::printf("threads launched %d, idle (outside the image) %d, pixels missed %d\n",
                launched, idle, missed);
    return 0;
}
