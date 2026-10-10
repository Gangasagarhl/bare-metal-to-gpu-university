// fbcheck.cc - F3-23: the screenshot test. Reads QEMU's screendump (a binary PPM file) and
// the kernel's serial log, replays the console text through the same FbConsole code into a
// reference image, and compares the two pixel by pixel. Also prints a zoomed text picture of
// the screendump's top-left corner, so a reader can see the pixels without an image viewer.
//   fbcheck <screen.ppm> <serial.txt> [--panic]
#include <cstdio>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include "fbconsole.h"

namespace {

bool read_ppm(const char* path, int& w, int& h, std::vector<uint32_t>& px)
{
    std::ifstream f(path, std::ios::binary);
    std::string magic;
    int maxval = 0;
    f >> magic >> w >> h >> maxval;
    f.get();                                       // the single whitespace after the header
    if (magic != "P6" || maxval != 255 || w <= 0 || h <= 0) {
        return false;
    }
    std::vector<unsigned char> rgb(static_cast<size_t>(w) * h * 3);
    f.read(reinterpret_cast<char*>(rgb.data()), static_cast<std::streamsize>(rgb.size()));
    px.resize(static_cast<size_t>(w) * h);
    for (size_t i = 0; i < px.size(); ++i) {
        px[i] = (uint32_t{rgb[3 * i]} << 16) | (uint32_t{rgb[3 * i + 1]} << 8) | rgb[3 * i + 2];
    }
    return static_cast<bool>(f);
}

uint32_t color_for(const std::string& line)
{
    if (line.find("] [DEBUG] ") != std::string::npos) return 0x00808890;
    if (line.find("] [ WARN] ") != std::string::npos) return 0x00f0d040;
    if (line.find("] [ERROR] ") != std::string::npos) return 0x00ff6060;
    return FbConsole::kText;
}

void zoom(const std::vector<uint32_t>& px, int w, int x0, int y0, int cols, int rows)
{
    for (int y = y0; y < y0 + rows; ++y) {
        std::string s;
        for (int x = x0; x < x0 + cols; ++x) {
            uint32_t c = px[static_cast<size_t>(y) * w + x];
            uint32_t lum = ((c >> 16) & 0xFF) + ((c >> 8) & 0xFF) + (c & 0xFF);
            s += lum > 300 ? '#' : '.';
        }
        std::printf("  %s\n", s.c_str());
    }
}

} // namespace

int main(int argc, char** argv)
{
    if (argc < 3) {
        std::fprintf(stderr, "usage: fbcheck screen.ppm serial.txt [--panic]\n");
        return 2;
    }
    int w = 0, h = 0;
    std::vector<uint32_t> shot;
    if (!read_ppm(argv[1], w, h, shot)) {
        std::printf("cannot read %s as a binary PPM\n", argv[1]);
        return 1;
    }
    std::printf("screendump: %d x %d pixels\n", w, h);
    if (argc > 3 && std::string(argv[3]) == "--panic") {
        size_t red = 0;
        for (uint32_t c : shot) {
            red += c == 0x00a01010;
        }
        std::printf("panic background pixels (0xa01010): %zu of %zu\n", red, shot.size());
        std::printf("top-left corner, 120 x 20 pixels ('#' = bright pixel):\n");
        zoom(shot, w, 0, 0, 120, 20);
        return red > shot.size() / 2 ? 0 : 1;
    }
    std::ifstream serial(argv[2]);
    std::vector<uint32_t> ref(static_cast<size_t>(w) * h);
    FbConsole con;
    con.init(ref.data(), w, h, w);
    std::string line;
    bool started = false;
    int replayed = 0;
    while (std::getline(serial, line)) {
        if (!started && line.rfind("console: started", 0) == 0) {
            started = true;
        }
        if (!started) {
            continue;
        }
        con.set_colors(color_for(line), FbConsole::kBackground);
        for (char c : line) {
            con.put(c);
        }
        con.set_colors(FbConsole::kText, FbConsole::kBackground);
        con.put('\n');
        ++replayed;
    }
    size_t diff = 0;
    int first_x = -1, first_y = -1;
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            if (shot[static_cast<size_t>(y) * w + x] != ref[static_cast<size_t>(y) * w + x]) {
                if (diff++ == 0) {
                    first_x = x;
                    first_y = y;
                }
            }
        }
    }
    std::printf("replayed %d serial lines into the reference image\n", replayed);
    std::printf("pixels that differ: %zu of %zu%s\n", diff, ref.size(), diff == 0 ? " (pixel-exact match)" : "");
    if (diff != 0) {
        std::printf("first difference at x=%d y=%d\n", first_x, first_y);
    }
    std::printf("top-left corner of the screendump, 120 x 20 pixels ('#' = bright pixel):\n");
    zoom(shot, w, 0, 0, 120, 20);
    return diff == 0 ? 0 : 1;
}
