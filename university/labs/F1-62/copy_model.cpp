// F1-62 Listing 1: a model of copies over the host link and kernels on the GPU.
// It is a model, not a measurement. Copy time = alpha + bytes / bandwidth.
// Modes:
//   serial   one queue: copy all inputs in, run all kernels, copy all results out
//   overlap  chunked pipeline: while chunk k computes, chunk k+1 copies in and
//            chunk k-1 copies out (needs page-locked host memory and separate copy engines)
//   resident data copied once, then `chunks` kernels run on data already in GPU memory
// Input lines: label mode chunks inMB outMB kernelMs linkGBps alphaUs
#include <algorithm>
#include <cstdio>
#include <iostream>
#include <string>
#include <vector>

struct Span { double start, end; };

void drawRow(const char* name, const std::vector<Span>& spans, double total)
{
    const int width = 72;
    std::string row(width, '.');
    for (const Span& s : spans) {
        int a = static_cast<int>(s.start / total * width);
        int b = std::max(a + 1, static_cast<int>(s.end / total * width));
        for (int x = a; x < b && x < width; ++x) { row[static_cast<std::size_t>(x)] = '#'; }
    }
    std::printf("  %-4s |%s|\n", name, row.c_str());
}

int main()
{
    std::string label, mode;
    int chunks = 0;
    double inMB = 0, outMB = 0, kernelMs = 0, gbps = 0, alphaUs = 0;
    while (std::cin >> label >> mode >> chunks >> inMB >> outMB >> kernelMs >> gbps >> alphaUs) {
        auto copyMs = [&](double mb) { return alphaUs / 1000.0 + mb * 1e6 / (gbps * 1e9) * 1000.0; };
        std::vector<Span> h2d, gpu, d2h;
        if (mode == "serial") {
            double t = 0;
            for (int k = 0; k < chunks; ++k) { h2d.push_back({t, t + copyMs(inMB)}); t = h2d.back().end; }
            for (int k = 0; k < chunks; ++k) { gpu.push_back({t, t + kernelMs}); t = gpu.back().end; }
            for (int k = 0; k < chunks; ++k) { d2h.push_back({t, t + copyMs(outMB)}); t = d2h.back().end; }
        } else if (mode == "overlap") {
            double inFree = 0, gpuFree = 0, outFree = 0;      // three engines, each in order
            for (int k = 0; k < chunks; ++k) {
                Span a{inFree, inFree + copyMs(inMB)};
                inFree = a.end;
                Span g{std::max(a.end, gpuFree), 0};
                g.end = g.start + kernelMs;
                gpuFree = g.end;
                Span o{std::max(g.end, outFree), 0};
                o.end = o.start + copyMs(outMB);
                outFree = o.end;
                h2d.push_back(a); gpu.push_back(g); d2h.push_back(o);
            }
        } else if (mode == "resident") {
            double t = copyMs(inMB);
            h2d.push_back({0, t});
            for (int k = 0; k < chunks; ++k) { gpu.push_back({t, t + kernelMs}); t = gpu.back().end; }
            d2h.push_back({t, t + copyMs(outMB)});
        } else {
            std::printf("%s: unknown mode %s\n", label.c_str(), mode.c_str());
            continue;
        }
        double total = std::max({h2d.back().end, gpu.back().end, d2h.back().end});
        double busy = 0;
        for (const Span& s : gpu) { busy += s.end - s.start; }
        std::printf("%s: mode %s, %d chunk(s), in %.0f MB, out %.0f MB, kernel %.2f ms each, link %.1f GB/s, alpha %.0f us\n",
                    label.c_str(), mode.c_str(), chunks, inMB, outMB, kernelMs, gbps, alphaUs);
        std::printf("  total %.2f ms, GPU busy %.2f ms = %.1f %% of the time\n", total, busy, 100.0 * busy / total);
        drawRow("H2D", h2d, total);
        drawRow("GPU", gpu, total);
        drawRow("D2H", d2h, total);
    }
    return 0;
}
