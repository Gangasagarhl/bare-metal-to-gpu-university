// HW301 answer-key arithmetic: every number quoted in university/_keys/HW301.keys.html for the
// final (parts A, B and D) and for the practical's paper part is recomputed here with the
// formulas of the chapters (F1-55 Little's law; F1-56 waves; F1-57 SIMD efficiency; F1-58
// occupancy and banks; F1-59 peak bandwidth; F1-60 loads in flight; F1-61 MMA counts;
// F1-62 alpha-beta model). The forensic numbers come from the model runs in this folder.
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>

static long ceilDiv(long a, long b) { return (a + b - 1) / b; }

static void efficiency(const char* name, long width, long lanesTrue, long instrTrue, long instrFalse, long common)
{
    const long lanesFalse = width - lanesTrue;
    long issued = common;
    long useful = common * width;
    if (lanesTrue > 0) { issued += instrTrue; useful += instrTrue * lanesTrue; }
    if (lanesFalse > 0) { issued += instrFalse; useful += instrFalse * lanesFalse; }
    std::printf("F7  %-28s width %2ld: issued %ld, useful %ld of %ld lane-slots = %.1f %%\n", name, width, issued, useful,
                issued * width, 100.0 * static_cast<double>(useful) / static_cast<double>(issued * width));
}

static long bankDegree(long threads, long pitch, long banks)
{
    // column read: thread t reads word t * pitch; distinct words per bank (F1-58 Listing 3, 4-byte words)
    long worst = 0;
    for (long b = 0; b < banks; ++b) {
        long n = 0;
        for (long t = 0; t < threads; ++t) {
            if ((t * pitch) % banks == b) { ++n; }
        }
        worst = std::max(worst, n);
    }
    return worst;
}

struct Occ { long byRegs, bySmem, byWarps, blocks; double occ; };
static Occ occupancy(long regsPerSm, long smemPerSm, long maxWarps, long maxBlocks, long regs, long smem, long threads, long width)
{
    const long wpb = ceilDiv(threads, width);
    Occ o{regsPerSm / (regs * threads), smem > 0 ? smemPerSm / smem : 999, maxWarps / wpb, 0, 0};
    o.blocks = std::min({o.byRegs, o.bySmem, o.byWarps, maxBlocks});
    o.occ = 100.0 * static_cast<double>(o.blocks * wpb) / static_cast<double>(maxWarps);
    return o;
}

int main()
{
    // F2: pipelined unit, latency 8 cycles, one issue per cycle; Little's law 300 cycles, one completion per 3 cycles
    std::printf("F2  200 dependent ops x 8 cycles = %ld cycles; 200 independent = %ld cycles (+ last latency = %ld)\n",
                200L * 8, 200L, 200L + 7);
    std::printf("F2  Little: rate 1/3 per cycle x 300 cycles = %.0f accesses in flight\n", 300.0 / 3.0);

    // F5: 60 SMs, 3 blocks per SM
    {
        const long slots = 60 * 3;
        for (long grid : {700L, 150L}) {
            const long waves = ceilDiv(grid, slots), last = grid - (waves - 1) * slots;
            std::printf("F5  grid %ld: slots %ld, waves %ld, last wave %ld of %ld, busy %.1f %%, idle slots in last wave %ld\n",
                        grid, slots, waves, last, slots, 100.0 * static_cast<double>(grid) / static_cast<double>(waves * slots), slots - last);
        }
    }

    // F7: if (c) {12} else {3} then 5 common
    efficiency("all 32 lanes true", 32, 32, 12, 3, 5);
    efficiency("30 of 32 true", 32, 30, 12, 3, 5);
    efficiency("62 of 64 true", 64, 62, 12, 3, 5);

    // F10: TG-1, 32 regs, 8192 B shared, 192 threads
    {
        const Occ o = occupancy(16384, 32768, 16, 4, 32, 8192, 192, 32);
        std::printf("F10 TG-1 32 regs / 8192 B / 192 thr: warps/block %ld; byRegs %ld, bySmem %ld, byWarps %ld, byBlocks 4 -> %ld blocks, %ld warps, %.1f %%\n",
                    ceilDiv(192, 32), o.byRegs, o.bySmem, o.byWarps, o.blocks, o.blocks * ceilDiv(192, 32), o.occ);
        const Occ o2 = occupancy(16384, 32768, 16, 4, 32, 8192, 128, 32);
        std::printf("F10 same kernel with 128 threads: byRegs %ld, bySmem %ld, byWarps %ld -> %ld blocks, %.1f %%\n", o2.byRegs, o2.bySmem, o2.byWarps, o2.blocks, o2.occ);
    }

    // F12: bank-conflict degrees for a 32-thread column read, 32 banks of 4 bytes
    for (long pitch : {32L, 33L, 48L, 64L}) {
        std::printf("F12 pitch %ld: column-read degree %ld\n", pitch, bankDegree(32, pitch, 32));
    }

    // F13: 320 bits at 10 GT/s; 4 arrays of 2^27 floats (3 read, 1 written)
    {
        const double peak = 320.0 / 8.0 * 10.0;              // GB/s
        const double bytes = 4.0 * 134217728.0 * 4.0;        // B
        std::printf("F13 peak %.1f GB/s = %.1f GiB/s; bytes %.0f = %.3f MB; bound %.3f ms; a 4.1 ms run would be %.2f x faster than the bound\n",
                    peak, peak * 1e9 / 1073741824.0, bytes, bytes / 1e6, bytes / (peak * 1e9) * 1e3, (bytes / (peak * 1e9) * 1e3) / 4.1);
    }

    // F15: latency 300 cycles, each load supports 12 instructions, 1 issue per cycle
    std::printf("F15 loads in flight = 300 / 12 = %.0f; warps with 1 load each: %ld; with 4 loads each: %ld\n", 300.0 / 12.0, 25L, ceilDiv(25, 4));

    // F17: 2048^3 multiply-adds
    {
        const double macs = 2048.0 * 2048.0 * 2048.0;
        std::printf("F17 MACs %.0f = 2^33; HMMA.16816 (2048 each) %.0f = 2^22; v_mfma 16x16x4 (1024 each) %.0f = 2^23; warp FFMA (32 each) %.0f = 2^28\n",
                    macs, macs / 2048, macs / 1024, macs / 32);
    }

    // F19: alpha 10 us, beta 16 GB/s
    {
        auto copyUs = [](double bytes) { return 10.0 + bytes / 16e9 * 1e6; };
        std::printf("F19 2000 copies of 32 KB: each %.3f us, total %.2f ms; one 64 MB copy: %.3f ms; ratio %.2f\n",
                    copyUs(32768), 2000 * copyUs(32768) / 1000.0, copyUs(67108864) / 1000.0, (2000 * copyUs(32768)) / copyUs(67108864));
        const double in = copyUs(128e6) / 1000.0, total = in + 1.0 + in;
        std::printf("F19 job: copy 128 MB %.2f ms + kernel 1 ms + copy %.2f ms = %.2f ms, GPU busy %.1f %%\n", in, in, total, 100.0 / total);
    }

    // D: TG-1, 65,536 threads, 20 registers, no shared memory: block sizes 64..1024
    for (long threads : {64L, 128L, 256L, 512L, 1024L}) {
        const Occ o = occupancy(16384, 32768, 16, 4, 20, 0, threads, 32);
        const long grid = ceilDiv(65536, threads);
        if (o.blocks == 0) { std::printf("D   block %4ld: does not fit\n", threads); continue; }
        const long slots = 4 * o.blocks, waves = ceilDiv(grid, slots);
        std::printf("D   block %4ld: byRegs %ld byWarps %ld byBlocks 4 -> %ld blocks, %.1f %%; grid %ld blocks, slots %ld, waves %ld, busy %.1f %%\n",
                    threads, o.byRegs, o.byWarps, o.blocks, o.occ, grid, slots, waves, 100.0 * static_cast<double>(grid) / static_cast<double>(waves * slots));
    }

    // Practical paper part: EX-1 stencil by hand (the program prints the same values)
    {
        const Occ o = occupancy(65536, 65536, 48, 16, 40, 0, 256, 32);
        const long grid = ceilDiv(1000000, 256), slots = 24 * o.blocks, waves = ceilDiv(grid, slots);
        std::printf("P   EX-1 stencil: warps/block %ld; byRegs %ld bySmem %ld byWarps %ld -> %ld blocks, %.1f %%; grid %ld, slots %ld, waves %ld, last %ld, busy %.1f %%; last block active %ld\n",
                    ceilDiv(256, 32), o.byRegs, o.bySmem, o.byWarps, o.blocks, o.occ, grid, slots, waves, grid - (waves - 1) * slots,
                    100.0 * static_cast<double>(grid) / static_cast<double>(waves * slots), 1000000 - (grid - 1) * 256);
        std::printf("P   EX-1 memory: 192/8 x 12 = %.1f GB/s; 48 MB -> %.3f us; copy 256 MB over 20 GB/s + 8 us = %.3f ms\n",
                    24.0 * 12.0, 48e6 / 288e9 * 1e6, 0.008 + 256e6 / 20e9 * 1e3);
    }
    return 0;
}
