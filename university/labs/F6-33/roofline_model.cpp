// F6-33 Listing 3: place kernels on the roofline of TG-1 (INVENTED teaching GPU).
// TG-1 memory roof: 256 GB/s (256-bit at 8 GT/s). TG-1 compute roof, a proposed
// extension of the invented device: 4 SMs x 32 lanes x 1 FMA per cycle x 2 FLOP
// x 1 GHz = 256 GFLOP/s. Ridge point = 256 / 256 = 1 FLOP/byte.
// Input: any lines containing "kernel <name> ... flops <F> bytes <B>" (per element),
// such as the output of count_sass.sh. Prints the roofline prediction for 2^24
// elements: attainable GFLOP/s, the bound, and the predicted time.
#include <algorithm>
#include <cstdio>
#include <iostream>
#include <sstream>
#include <string>

int main()
{
    const double peakFlops = 4.0 * 32.0 * 2.0 * 1.0e9;   // FLOP/s
    const double peakBytes = 256.0e9;                     // byte/s
    const double n = 16777216.0;                          // 2^24 elements
    std::printf("TG-1 (invented): compute roof %.0f GFLOP/s, memory roof %.0f GB/s, "
                "ridge %.2f FLOP/B\n",
                peakFlops / 1e9, peakBytes / 1e9, peakFlops / peakBytes);
    std::printf("%-30s %6s %6s %9s %11s %8s %10s\n", "kernel", "FLOP", "bytes", "AI FLOP/B",
                "attain GF/s", "bound", "time us");
    std::string line;
    while (std::getline(std::cin, line)) {
        std::istringstream in(line);
        std::string word, name;
        double flops = -1.0, bytes = -1.0;
        while (in >> word) {
            if (word == "kernel") { in >> name; }
            else if (word == "flops") { in >> flops; }
            else if (word == "bytes") { in >> bytes; }
        }
        if (name.empty() || flops < 0.0 || bytes <= 0.0) { continue; }
        const double ai = flops / bytes;
        const double attain = std::min(peakFlops, ai * peakBytes);
        const double tCompute = flops * n / peakFlops;
        const double tMemory = bytes * n / peakBytes;
        const double t = std::max(tCompute, tMemory);
        std::printf("%-30s %6.0f %6.0f %9.3f %11.1f %8s %10.1f\n", name.c_str(), flops, bytes, ai,
                    attain / 1e9, tCompute > tMemory ? "compute" : "memory", t * 1e6);
    }
    return 0;
}
