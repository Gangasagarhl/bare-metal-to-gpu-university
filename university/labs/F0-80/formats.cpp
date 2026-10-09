// F0-80 Listing 1: the reduced-precision formats side by side (from the model in lowp.hpp).
#include <cstdio>

#include "lowp.hpp"

int main()
{
    const Format fs[] = {kBinary32, kBinary16, kBfloat16, kE5M2, kE4M3};
    std::printf("%-17s %-3s %-11s %-12s %-12s %-12s\n", "format", "p", "u = 2^-p", "max finite",
                "min normal", "min subnorm");
    for (const Format& f : fs) {
        std::printf("%-17s %-3d %-11.4g %-12.6g %-12.4g %-12.4g\n", f.name, f.p, unitRoundoff(f),
                    maxFinite(f), minNormal(f), minSubnormal(f));
    }
    const double xs[] = {0.1, 1.0 / 3.0, 3.14159265358979, 1000.3, 70000.0, 1.0e-5};
    std::printf("\n%-14s", "x");
    for (const Format& f : fs) {
        std::printf(" %-16.16s", f.name);
    }
    std::printf("\n");
    for (double x : xs) {
        std::printf("%-14.10g", x);
        for (const Format& f : fs) {
            std::printf(" %-16.9g", roundTo(x, f));
        }
        std::printf("\n");
    }
    return 0;
}
