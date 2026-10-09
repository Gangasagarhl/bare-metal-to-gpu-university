// F1-58 Listing 4: occupancy arithmetic. How many warps of a kernel fit on one SM?
// Input lines: label regsPerThread sharedBytesPerBlock threadsPerBlock
// The SM limits below are those of TG-1, the university's INVENTED teaching GPU.
// Replace them with the numbers of your GPU from its device query and vendor guide.
#include <algorithm>
#include <cstdio>
#include <iostream>
#include <string>

struct SmLimits
{
    long registers = 16384;      // 32-bit registers per SM          (TG-1, invented)
    long sharedBytes = 32768;    // shared memory per SM in bytes    (TG-1, invented)
    long maxWarps = 16;          // resident warps per SM            (TG-1, invented)
    long maxBlocks = 4;          // resident blocks per SM           (TG-1, invented)
    long warpWidth = 32;         // threads per warp                 (TG-1, invented)
};

int main()
{
    const SmLimits sm;
    std::string label;
    long regs = 0, shared = 0, threads = 0;
    std::printf("%-22s %5s %7s %7s | %6s %6s %6s %6s | %6s %-18s %s\n", "kernel", "regs", "smem", "thr/blk",
                "byReg", "bySmem", "byWarp", "byBlk", "blocks", "limited by", "occupancy");
    while (std::cin >> label >> regs >> shared >> threads) {
        const long warpsPerBlock = (threads + sm.warpWidth - 1) / sm.warpWidth;
        const long byRegs = sm.registers / (regs * threads);              // blocks allowed by registers
        const long bySmem = shared > 0 ? sm.sharedBytes / shared : 999;          // 999 = no shared-memory limit
        const long byWarps = sm.maxWarps / warpsPerBlock;
        const long blocks = std::min({byRegs, bySmem, byWarps, sm.maxBlocks});
        std::string limiter;                                              // every limit that is reached
        if (blocks == byRegs) { limiter += "regs "; }
        if (blocks == bySmem) { limiter += "smem "; }
        if (blocks == byWarps) { limiter += "warps "; }
        if (blocks == sm.maxBlocks) { limiter += "blocks"; }
        const double occ = 100.0 * static_cast<double>(blocks * warpsPerBlock) / static_cast<double>(sm.maxWarps);
        std::printf("%-22s %5ld %7ld %7ld | %6ld %6ld %6ld %6ld | %6ld %-18s %5.1f %%\n", label.c_str(), regs, shared,
                    threads, byRegs, bySmem, byWarps, sm.maxBlocks, blocks, limiter.c_str(), occ);
    }
    return 0;
}
