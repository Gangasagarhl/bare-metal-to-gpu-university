// HW301 practical (P), REFERENCE SOLUTION: map a kernel's threads onto a GPU using only
// its documentation sheet. Candidates receive kernel_map_start.cpp (same file, six TODOs
// emptied) and kernel_map_start.in (owner ruling B7).
//
// Input (stdin). One documentation sheet, then one line per kernel:
//   gpu <name> <SMs> <warpWidth> <maxWarpsPerSM> <maxBlocksPerSM> <registersPerSM> <sharedBytesPerSM>
//       <memoryBusBits> <memoryDataRateGTps> <hostLinkGBps> <hostLinkAlphaUs>
//   kernel <label> <threadsPerBlock> <gridThreads> <registersPerThread> <sharedBytesPerBlock>
//       <deviceBytesMovedMB> <hostCopyMB>
// Every sheet value is an EXERCISE VALUE (owner ruling A4): the exam GPUs EX-1 and EX-2 are
// invented, like the course's TG-1. On the lab GPU the sheet is replaced by the device query
// and the vendor document (tag each row DOC/TOOL), and the kernel numbers by the compiler's
// resource report (F1-58).
//
// The self-check runs the published TG-1 cases of F1-56, F1-58, F1-59, F1-62 and F1-63
// (their answers are printed in those chapters) before the sheet is processed. Exit code 0
// only when the self-check reports 0 failures.
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <iostream>
#include <string>

struct Gpu
{
    std::string name;
    long sms = 0, warpWidth = 0, maxWarps = 0, maxBlocks = 0, registers = 0, sharedBytes = 0;
    long busBits = 0;
    double gtps = 0, linkGBps = 0, alphaUs = 0;
};

struct Kernel
{
    std::string label;
    long threads = 0, gridThreads = 0, regs = 0, shared = 0;
    double deviceMB = 0, copyMB = 0;
};

struct Fit { long byRegs = 0, bySmem = 0, byWarps = 0, blocks = 0; std::string limiter; };
struct Waves { long slots = 0, waves = 0, last = 0; double busy = 0; };
struct Tail { long gridBlocks = 0, activeInLast = 0, idleLanesInLast = 0; };

// ---- the six functions a candidate completes --------------------------------------------

// TODO 1: warps (or wavefronts) per block, rounded up (F1-57: a block of 100 threads is 4 warps of 32).
long warpsPerBlock(long threads, long warpWidth)
{
    return (threads + warpWidth - 1) / warpWidth;
}

// TODO 2: resident blocks per SM and the limiting resource(s) (F1-58 Listing 4).
//   byRegs  = registers per SM / (registers per thread * threads per block)
//   bySmem  = shared per SM / shared per block, or 999 when the kernel uses none
//   byWarps = max warps per SM / warps per block
//   blocks  = the smallest of byRegs, bySmem, byWarps and the SM's block limit
//   limiter = every limit that equals blocks, in the order "regs smem warps blocks"
Fit blocksPerSm(const Gpu& g, const Kernel& k)
{
    Fit f;
    const long wpb = warpsPerBlock(k.threads, g.warpWidth);
    f.byRegs = g.registers / (k.regs * k.threads);
    f.bySmem = k.shared > 0 ? g.sharedBytes / k.shared : 999;
    f.byWarps = g.maxWarps / wpb;
    f.blocks = std::min({f.byRegs, f.bySmem, f.byWarps, g.maxBlocks});
    if (f.blocks == f.byRegs) { f.limiter += "regs "; }
    if (f.blocks == f.bySmem) { f.limiter += "smem "; }
    if (f.blocks == f.byWarps) { f.limiter += "warps "; }
    if (f.blocks == g.maxBlocks) { f.limiter += "blocks"; }
    return f;
}

// TODO 3: occupancy in percent = resident warps / max warps per SM (F1-58).
double occupancyPercent(const Gpu& g, long blocks, long warpsPerBlk)
{
    return 100.0 * static_cast<double>(blocks * warpsPerBlk) / static_cast<double>(g.maxWarps);
}

// TODO 4: waves of a launch (F1-56 Listing 3): slots = SMs * blocks per SM; waves rounded up;
//   blocks in the last wave; busy % = grid blocks / (waves * slots).
Waves wavesOf(const Gpu& g, long blocksPerSmCount, long gridBlocks)
{
    Waves w;
    w.slots = g.sms * blocksPerSmCount;
    w.waves = (gridBlocks + w.slots - 1) / w.slots;
    w.last = gridBlocks - (w.waves - 1) * w.slots;
    w.busy = 100.0 * static_cast<double>(gridBlocks) / static_cast<double>(w.waves * w.slots);
    return w;
}

// TODO 5: the tail block (F1-63 worked example): grid blocks rounded up; threads active in the
//   last block; lanes of the last block that hold no thread (warps are issued whole).
Tail tailOf(const Kernel& k)
{
    Tail t;
    t.gridBlocks = (k.gridThreads + k.threads - 1) / k.threads;
    t.activeInLast = k.gridThreads - (t.gridBlocks - 1) * k.threads;
    t.idleLanesInLast = k.threads - t.activeInLast;
    return t;
}

// TODO 6: bandwidth and the host link (F1-59, F1-62).
//   peak GB/s = bus bits / 8 * data rate (GT/s);  bound in us = bytes / peak;
//   copy ms   = alpha + bytes / link bandwidth (the alpha-beta model).
double peakGBps(long busBits, double gtps) { return static_cast<double>(busBits) / 8.0 * gtps; }
double minTimeUs(double megabytes, double peakGBpsValue) { return megabytes * 1e6 / (peakGBpsValue * 1e9) * 1e6; }
double copyMs(double megabytes, double linkGBps, double alphaUs)
{
    return alphaUs / 1000.0 + megabytes * 1e6 / (linkGBps * 1e9) * 1000.0;
}

// ---- self-check against the chapters' published TG-1 cases (do not edit) ----------------
int selfCheck()
{
    int failures = 0;
    auto expectL = [&](const char* what, long got, long want) {
        if (got != want) { std::printf("  self-check FAIL %-40s got %ld, expected %ld\n", what, got, want); ++failures; }
    };
    auto expectD = [&](const char* what, double got, double want, double tol) {
        if (std::fabs(got - want) > tol) { std::printf("  self-check FAIL %-40s got %.4f, expected %.4f\n", what, got, want); ++failures; }
    };
    auto expectS = [&](const char* what, const std::string& got, const std::string& want) {
        if (got != want) { std::printf("  self-check FAIL %-40s got '%s', expected '%s'\n", what, got.c_str(), want.c_str()); ++failures; }
    };
    const Gpu tg1{"TG-1", 4, 32, 16, 4, 16384, 32768, 256, 8.0, 16.0, 10.0};   // invented (F1-55 note)
    expectL("warps per block, 256 threads of 32 (F1-57)", warpsPerBlock(256, 32), 8);
    expectL("warps per block, 100 threads of 32 (F1-57)", warpsPerBlock(100, 32), 4);
    expectL("wavefronts per block, 100 threads of 64 (F1-57)", warpsPerBlock(100, 64), 2);
    const Kernel examP{"examP", 256, 10000, 24, 4224, 0, 0};                   // F1-63 worked example
    Fit f = blocksPerSm(tg1, examP);
    expectL("F1-63 worked example: blocks per SM", f.blocks, 2);
    expectS("F1-63 worked example: limiter", f.limiter, "regs warps ");
    expectD("F1-63 worked example: occupancy %", occupancyPercent(tg1, f.blocks, 8), 100.0, 0.01);
    const Kernel worked58{"worked", 128, 0, 40, 6144, 0, 0};                    // F1-58 worked example
    f = blocksPerSm(tg1, worked58);
    expectL("F1-58 worked example: blocks per SM", f.blocks, 3);
    expectS("F1-58 worked example: limiter", f.limiter, "regs ");
    expectD("F1-58 worked example: occupancy %", occupancyPercent(tg1, f.blocks, 4), 75.0, 0.01);
    const Kernel sharedHeavy{"shared_heavy", 128, 0, 24, 16384, 0, 0};          // F1-58 occupancy.out
    f = blocksPerSm(tg1, sharedHeavy);
    expectL("F1-58 shared_heavy: blocks per SM", f.blocks, 2);
    expectS("F1-58 shared_heavy: limiter", f.limiter, "smem ");
    Waves w = wavesOf(tg1, 2, 9);                                               // F1-56 waves.out
    expectL("F1-56 tg1_grid_9: waves", w.waves, 2);
    expectL("F1-56 tg1_grid_9: last wave", w.last, 1);
    expectD("F1-56 tg1_grid_9: busy %", w.busy, 56.25, 0.01);
    w = wavesOf(tg1, 2, 100);
    expectL("F1-56 tg1_grid_100: waves", w.waves, 13);
    expectL("F1-56 tg1_grid_100: last wave", w.last, 4);
    Tail t = tailOf(examP);                                                     // F1-63 worked example
    expectL("F1-63 worked example: grid blocks", t.gridBlocks, 40);
    expectL("F1-63 worked example: active in last block", t.activeInLast, 16);
    expectL("F1-63 worked example: idle lanes in last block", t.idleLanesInLast, 240);
    expectD("F1-59 TG-1 peak GB/s (256 bits, 8 GT/s)", peakGBps(256, 8.0), 256.0, 1e-9);
    expectD("F1-59 SAXPY bound us (805.306 MB)", minTimeUs(805.306368, 256.0), 3145.728, 0.001);
    expectD("F1-62 copy ms (256 MB, 16 GB/s, 10 us)", copyMs(256, 16.0, 10.0), 16.01, 1e-6);
    std::printf("self-check: %d failures\n", failures);
    return failures;
}

int main()
{
    const int failures = selfCheck();
    std::string word;
    Gpu g;
    if (!(std::cin >> word >> g.name >> g.sms >> g.warpWidth >> g.maxWarps >> g.maxBlocks >> g.registers >> g.sharedBytes
          >> g.busBits >> g.gtps >> g.linkGBps >> g.alphaUs) || word != "gpu") {
        std::printf("no documentation sheet on stdin (first line must start with 'gpu')\n");
        return 1;
    }
    const double peak = peakGBps(g.busBits, g.gtps);
    std::printf("sheet %s (exercise values): %ld SMs, warp %ld, per SM %ld warps / %ld blocks / %ld regs / %ld B shared;"
                " memory %ld bits x %.1f GT/s = %.1f GB/s; host link %.1f GB/s, alpha %.0f us\n",
                g.name.c_str(), g.sms, g.warpWidth, g.maxWarps, g.maxBlocks, g.registers, g.sharedBytes, g.busBits, g.gtps,
                peak, g.linkGBps, g.alphaUs);
    Kernel k;
    while (std::cin >> word >> k.label >> k.threads >> k.gridThreads >> k.regs >> k.shared >> k.deviceMB >> k.copyMB) {
        if (word != "kernel" || k.threads <= 0 || k.gridThreads <= 0 || k.regs <= 0 || k.shared < 0) {
            std::printf("%s: invalid input line\n", k.label.c_str());
            continue;
        }
        const long wpb = warpsPerBlock(k.threads, g.warpWidth);
        const Fit f = blocksPerSm(g, k);
        const Tail t = tailOf(k);
        std::printf("%s: %ld threads/block = %ld warps; grid %ld threads = %ld blocks (last block: %ld active, %ld idle lanes)\n",
                    k.label.c_str(), k.threads, wpb, k.gridThreads, t.gridBlocks, t.activeInLast, t.idleLanesInLast);
        std::printf("  per SM: byRegs %ld, bySmem %ld, byWarps %ld, byBlocks %ld -> %ld block(s), limited by %s\n",
                    f.byRegs, f.bySmem, f.byWarps, g.maxBlocks, f.blocks, f.blocks == 0 ? "(does not fit)" : f.limiter.c_str());
        if (f.blocks == 0) {
            std::printf("  cannot launch on %s: not even one block fits an SM (%s)\n", g.name.c_str(),
                        f.byRegs == 0 ? "registers" : f.bySmem == 0 ? "shared memory" : "warp slots");
        } else {
            const Waves w = wavesOf(g, f.blocks, t.gridBlocks);
            std::printf("  occupancy %.1f %% (%ld of %ld warps); slots %ld, waves %ld, last wave %ld of %ld, busy %.1f %%\n",
                        occupancyPercent(g, f.blocks, wpb), f.blocks * wpb, g.maxWarps, w.slots, w.waves, w.last, w.slots, w.busy);
        }
        std::printf("  memory: %.3f MB at %.1f GB/s peak -> at least %.3f us; host copy %.3f MB -> %.3f ms\n",
                    k.deviceMB, peak, minTimeUs(k.deviceMB, peak), k.copyMB, copyMs(k.copyMB, g.linkGBps, g.alphaUs));
    }
    return failures == 0 ? 0 : 1;
}
