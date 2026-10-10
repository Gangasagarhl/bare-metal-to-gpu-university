// F6-27 Listing 2: the GEMM hierarchy as arithmetic. A block tile (BM x BN x BK) is split
// into warp tiles (WM x WN), each made of instruction tiles (IM x IN x IK). The program
// prints, for each configuration, the counts a designer checks before writing any code.
// Configurations are examples for the arithmetic, not tuned settings for any GPU.
#include <cstdio>

struct Config
{
    const char* name;
    int BM, BN, BK, WM, WN, IM, IN, IK, bytesPerElem, stages;
};

int main()
{
    const Config configs[] = {
        {"F6-23 vec 128x128 (FP32, 1 instr = 1 FMA)", 128, 128, 8, 16, 128, 1, 1, 1, 4, 1},
        {"F6-24 dbuf 64x64  (FP32)", 64, 64, 8, 8, 64, 1, 1, 1, 4, 2},
        {"F6-26 wmma 64x64  (FP16, 16x16x16, no smem)", 64, 64, 16, 32, 32, 16, 16, 16, 2, 0},
        {"example 128x128x32 (FP16, 16x8x16, 3 stages)", 128, 128, 32, 64, 64, 16, 8, 16, 2, 3},
    };
    std::printf("%-46s %5s %8s %10s %12s %12s\n", "configuration", "warps", "instr/kk",
                "smem bytes", "FLOP/B warp", "FLOP/B block");
    for (const Config& c : configs) {
        const int warps = (c.BM / c.WM) * (c.BN / c.WN);
        // instruction tiles per warp per BK step
        const int instr = (c.WM / c.IM) * (c.WN / c.IN) * (c.BK / c.IK);
        const int smem = c.stages * (c.BM + c.BN) * c.BK * c.bytesPerElem;
        // per BK step: each warp reads its WM x BK of A and BK x WN of B (from shared memory,
        // or from global memory and L1 when there is no shared-memory stage); the block as a
        // whole brings BM x BK of A and BK x BN of B in from global memory
        const double flop = 2.0 * c.BM * c.BN * c.BK;
        const double smemBytes = double(warps) * (c.WM + c.WN) * c.BK * c.bytesPerElem;
        const double globBytes = double(c.BM + c.BN) * c.BK * c.bytesPerElem;
        std::printf("%-46s %5d %8d %10d %12.1f %12.1f\n", c.name, warps, instr, smem,
                    flop / smemBytes, flop / globBytes);
    }
    std::printf("warp tile of the F6-23/F6-24 kernels: tid / (BN/TN) picks the rows, so one warp "
                "covers 32/(BN/TN) thread rows across the whole BN\n");
    std::printf("instr/kk = instruction tiles per warp per BK step; FLOP/B warp = operations per "
                "operand byte all warps read per step; FLOP/B block = per byte the block brings "
                "in from global memory\n");
    return 0;
}
