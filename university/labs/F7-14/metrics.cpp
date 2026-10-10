// F7-14 Listing 2: the arithmetic behind a profiler's derived metrics and roofline view.
// Input: one line per kernel with RAW totals a profiler reports (time, bytes moved, floating-
// point operations), plus one MACHINE line with the two roofs. All numbers in metrics.in are
// INVENTED model values for practice; replace them with your own profiler's output.
#include <algorithm>
#include <cstdio>
#include <iostream>
#include <sstream>
#include <string>

int main()
{
    double peakGBs = 0.0;      // memory roof, GB/s (decimal)
    double peakGFs = 0.0;      // compute roof, GFLOP/s
    std::string line;
    std::printf("%-12s %9s %9s %9s %10s %11s %8s %s\n", "kernel", "time us", "GB/s", "GFLOP/s",
                "FLOP/byte", "roof GF/s", "of roof", "bound by");
    while (std::getline(std::cin, line)) {
        if (line.empty() || line[0] == '#') {
            continue;
        }
        std::istringstream in(line);
        std::string name;
        in >> name;
        if (name == "MACHINE") {
            in >> peakGBs >> peakGFs;
            std::printf("machine model: memory roof %.0f GB/s, compute roof %.0f GFLOP/s, ridge %.2f FLOP/byte\n",
                        peakGBs, peakGFs, peakGFs / peakGBs);
            continue;
        }
        double us = 0, bytesRead = 0, bytesWritten = 0, flops = 0;
        in >> us >> bytesRead >> bytesWritten >> flops;
        const double seconds = us * 1e-6;
        const double bytes = bytesRead + bytesWritten;
        const double gbs = bytes / seconds / 1e9;
        const double gfs = flops / seconds / 1e9;
        const double intensity = flops / bytes;                      // FLOP per byte of DRAM traffic
        const double roof = std::min(peakGFs, peakGBs * intensity);  // the roofline at this intensity
        const char* bound = (peakGBs * intensity < peakGFs) ? "memory" : "compute";
        std::printf("%-12s %9.1f %9.1f %9.1f %10.3f %11.1f %7.1f%% %s\n", name.c_str(), us, gbs, gfs,
                    intensity, roof, 100.0 * gfs / roof, bound);
    }
    return 0;
}
