// F7-16 Listing 2: what a tile shape costs and buys (arithmetic only, no GPU).
#include <cstdio>

struct Tile { const char* name; int BM; int BN; int BK; int TM; int TN; int pad; };

int main()
{
    const Tile tiles[] = {
        {"16x16x16, 1x1 (sgemmLds)", 16, 16, 16, 1, 1, 0},
        {"64x64x16, 4x4 (sgemmLdsReg)", 64, 64, 16, 4, 4, 4},
        {"128x64x16, 8x4", 128, 64, 16, 8, 4, 4},
        {"128x128x8, 8x8", 128, 128, 8, 8, 8, 4},
    };
    const long long M = 4096, N = 4096;
    const int ldsLimit = 65536;   // per-workgroup limit enforced by the compiler in this build (gfx90a)
    std::printf("naive kernel: 2 FLOP per 2 loads of 4 bytes = %.2f FLOP/byte from global memory\n",
                2.0 / 8.0);
    std::printf("%-28s %7s %9s %10s %10s %8s %9s %9s\n", "tile (BMxBNxBK, TMxTN)", "threads",
                "LDS bytes", "FLOP/byte", "LDS rd/FMA", "acc regs", "WGs 4096^2", "WGs/limit");
    for (const Tile& t : tiles) {
        const int threads = (t.BM / t.TM) * (t.BN / t.TN);
        const int lds = (t.BK * (t.BM + t.pad) + t.BK * t.BN) * 4;
        // per k-tile: the workgroup loads (BM + BN) * BK floats and does BM * BN * BK FMAs
        const double flopPerByte = (2.0 * t.BM * t.BN * t.BK) / ((t.BM + t.BN) * t.BK * 4.0);
        // per k step: each thread reads TM + TN floats from LDS and does TM * TN FMAs
        const double ldsPerFma = static_cast<double>(t.TM + t.TN) / (t.TM * t.TN);
        const long long wgs = ((M + t.BM - 1) / t.BM) * ((N + t.BN - 1) / t.BN);
        std::printf("%-28s %7d %9d %10.2f %10.3f %8d %9lld %9d\n", t.name, threads, lds, flopPerByte,
                    ldsPerFma, t.TM * t.TN, wgs, ldsLimit / lds);
    }
    return 0;
}
