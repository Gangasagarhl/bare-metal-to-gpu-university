// roofline.cc - place measured kernels under the measured roofs (Williams, Waterman, Patterson).
// Input (made by run.sh from the other programs' output):
//   ROOF peak1|peakN|bw1|bwN|threads <value>
//   KERNEL <name> <threads> <flops> <bytes> <measured GFLOP/s>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <iostream>
#include <map>
#include <string>
#include <vector>

struct Kernel
{
    std::string name;
    int threads;
    double flops;
    double bytes;
    double gflops;
};

int main()
{
    std::map<std::string, double> roof;
    std::vector<Kernel> ks;
    std::string tag;
    while (std::cin >> tag) {
        if (tag == "ROOF") {
            std::string key;
            double v = 0.0;
            std::cin >> key >> v;
            roof[key] = v;
        } else if (tag == "KERNEL") {
            Kernel k;
            std::cin >> k.name >> k.threads >> k.flops >> k.bytes >> k.gflops;
            ks.push_back(k);
        }
    }
    double const p1 = roof["peak1"];
    double const b1 = roof["bw1"];
    double const pN = roof["peakN"];
    double const bN = roof["bwN"];
    std::printf("roofs: 1 core  %.1f GFLOP/s, %.1f GB/s, ridge point %.2f FLOP/byte\n", p1, b1, p1 / b1);
    std::printf("       %d cores %.1f GFLOP/s, %.1f GB/s, ridge point %.2f FLOP/byte\n\n",
                static_cast<int>(roof["threads"]), pN, bN, pN / bN);
    std::printf("%-3s %-20s %3s %10s %10s %10s %7s  %s\n", "id", "kernel", "thr", "AI", "roof",
                "measured", "of roof", "bound by");
    char id = 'a';
    for (auto const& k : ks) {
        double const ai = k.flops / k.bytes;  // FLOP per byte of compulsory traffic
        double const peak = k.threads > 1 ? pN : p1;
        double const bw = k.threads > 1 ? bN : b1;
        double const attainable = std::min(peak, ai * bw);
        std::printf("%-3c %-20s %3d %10.2f %10.1f %10.1f %6.0f %%  %s\n", id++, k.name.c_str(), k.threads,
                    ai, attainable, k.gflops, 100.0 * k.gflops / attainable,
                    ai * bw < peak ? "memory" : "compute");
    }
    // ASCII chart, both axes logarithmic (base 2). Single-core roof drawn with '*'.
    int const W = 64;
    int const H = 18;
    double const xMin = -3.0;  // AI from 2^-3 = 0.125 ...
    double const xMax = 8.0;   // ... to 2^8 = 256 FLOP/byte
    double const yMin = -1.0;  // GFLOP/s from 2^-1 ...
    double const yMax = std::ceil(std::log2(std::max(pN, p1))) + 1.0;
    std::vector<std::string> g(H, std::string(W, ' '));
    for (int c = 0; c < W; ++c) {
        double const ai = std::exp2(xMin + (xMax - xMin) * c / (W - 1));
        double const y = std::log2(std::min(p1, ai * b1));
        int const r = static_cast<int>(std::lround((yMax - y) / (yMax - yMin) * (H - 1)));
        if (r >= 0 && r < H) {
            g[r][c] = '*';
        }
    }
    id = 'a';
    for (auto const& k : ks) {
        double const x = std::log2(k.flops / k.bytes);
        double const y = std::log2(k.gflops);
        int const c = static_cast<int>(std::lround((x - xMin) / (xMax - xMin) * (W - 1)));
        int const r = static_cast<int>(std::lround((yMax - y) / (yMax - yMin) * (H - 1)));
        if (c >= 0 && c < W && r >= 0 && r < H) {
            g[r][c] = id;
        }
        ++id;
    }
    std::printf("\nGFLOP/s (log2 scale; '*' = single-core roof; letters = kernels above;\n");
    std::printf("         a later letter overwrites an earlier one at the same spot: the table rules)\n");
    for (int r = 0; r < H; ++r) {
        double const y = std::exp2(yMax - (yMax - yMin) * r / (H - 1));
        std::printf("%7.1f |%s\n", y, g[r].c_str());
    }
    std::printf("        +%s\n", std::string(W, '-').c_str());
    std::printf("         0.125     arithmetic intensity (FLOP/byte, log2 scale)        256\n");
    return 0;
}
