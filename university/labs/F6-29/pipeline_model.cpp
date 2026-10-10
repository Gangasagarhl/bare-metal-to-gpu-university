// F6-29 Listing 2: plan a chunked pipeline before you build it.
// For the invented teaching GPU TG-1 (host link 16 GB/s, 10 us fixed cost per copy)
// it compares, for several chunk counts:
//   formula  serial/C + (C-1)/C * max(a, k, b)   (equal stages, no fixed cost)
//   model    a simulation with the fixed cost per copy, 1 or 2 copy engines, and
//            the order in which the host issues the work (depth-first: in, kernel,
//            out per chunk; breadth-first: all ins, then all kernels, then all outs).
// Model assumption (not a vendor fact): each engine runs its work in issue order.
// Input lines: label inMiB outMiB kernelMs copyEngines depth|breadth C1 C2 ... 0
#include <algorithm>
#include <cstdio>
#include <iostream>
#include <string>
#include <vector>

const double linkBytesPerMs = 16.0e9 / 1000.0;          // TG-1 (invented): 16 GB/s
const double fixedMs = 0.010;                           // TG-1 (invented): 10 us per copy

double copyMs(double bytes) { return fixedMs + bytes / linkBytesPerMs; }

struct Work
{
    int chunk;
    char kind;   // 'i' copy in, 'k' kernel, 'o' copy out
};

double simulate(double inB, double outB, double kMs, int engines, bool depth, int chunks)
{
    std::vector<Work> order;
    if (depth) {
        for (int c = 0; c < chunks; ++c) {
            order.push_back({c, 'i'});
            order.push_back({c, 'k'});
            order.push_back({c, 'o'});
        }
    } else {
        for (char kind : {'i', 'k', 'o'}) {
            for (int c = 0; c < chunks; ++c) { order.push_back({c, kind}); }
        }
    }
    std::vector<double> inEnd(static_cast<std::size_t>(chunks), 0.0);
    std::vector<double> kEnd(static_cast<std::size_t>(chunks), 0.0);
    double copyInFree = 0.0, copyOutFree = 0.0, computeFree = 0.0, total = 0.0;
    for (const Work& w : order) {
        const std::size_t c = static_cast<std::size_t>(w.chunk);
        if (w.kind == 'i') {
            const double start = copyInFree;
            inEnd[c] = start + copyMs(inB / chunks);
            copyInFree = inEnd[c];
            if (engines == 1) { copyOutFree = copyInFree; }      // one shared engine
        } else if (w.kind == 'k') {
            const double start = std::max(computeFree, inEnd[c]);
            kEnd[c] = start + kMs / chunks;
            computeFree = kEnd[c];
        } else {
            const double start = std::max(copyOutFree, kEnd[c]);
            const double end = start + copyMs(outB / chunks);
            copyOutFree = end;
            if (engines == 1) { copyInFree = copyOutFree; }
            total = std::max(total, end);
        }
    }
    return total;
}

int main()
{
    std::string label, order;
    double inMiB = 0.0, outMiB = 0.0, kMs = 0.0;
    int engines = 0;
    while (std::cin >> label >> inMiB >> outMiB >> kMs >> engines >> order) {
        const double inB = inMiB * 1024.0 * 1024.0;
        const double outB = outMiB * 1024.0 * 1024.0;
        const double a = copyMs(inB);
        const double b = copyMs(outB);
        const double serial = a + kMs + b;
        std::printf("%s: copy in %.3f ms, kernel %.3f ms, copy out %.3f ms, serial %.3f ms\n",
                    label.c_str(), a, kMs, b, serial);
        std::printf("  copy engines %d, issue order %s\n", engines, order.c_str());
        std::printf("  %6s %11s %11s %9s\n", "chunks", "formula ms", "model ms", "speed-up");
        int c = 0;
        while (std::cin >> c && c > 0) {
            const double formula = serial / c + (c - 1) * std::max({a, kMs, b}) / c;
            const double model = simulate(inB, outB, kMs, engines, order == "depth", c);
            std::printf("  %6d %11.3f %11.3f %8.2fx\n", c, formula, model, serial / model);
        }
    }
    return 0;
}
