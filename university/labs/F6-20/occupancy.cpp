// F6-20 Listing 1: how many blocks of a kernel fit on one SM, computed two ways.
// (1) our own small model of the four limits (registers, shared memory, warps, blocks);
// (2) the occupancy calculator in the CUDA Toolkit header cuda_occupancy.h (host code).
// The device numbers below are INPUTS chosen for the example, not the specification
// of any product: replace them with what deviceQuery prints for your GPU (F6-07).
#include <cuda_occupancy.h>
#include <algorithm>
#include <climits>
#include <cstdio>
#include <string>

struct Device            // the fields an occupancy calculation needs
{
    int computeMajor, computeMinor;
    int maxThreadsPerSM, maxBlocksPerSM, regsPerSM, regsPerBlock;
    int smemPerSM, reservedSmemPerBlock, maxThreadsPerBlock;
};

struct Limits
{
    int byRegs, bySmem, byWarps, byBlocks, active;
    std::string limiter;
};

int roundUp(int x, int g)
{
    return (x + g - 1) / g * g;
}

// Our model. Allocation units (256 registers per warp, 128 bytes of shared memory,
// 4 sub-partitions) are those that cuda_occupancy.h of CUDA 12.0 uses for compute
// capability 8.x; read the header to see them.
Limits ourModel(const Device& d, int threads, int regsPerThread, int smemPerBlock)
{
    const int warpSize = 32;
    const int warps = (threads + warpSize - 1) / warpSize;
    const int regsPerWarp = roundUp(regsPerThread * warpSize, 256);
    const int warpsPerSubPartition = (d.regsPerSM / 4) / regsPerWarp;
    Limits l{};
    l.byRegs = regsPerThread == 0 ? INT_MAX : (warpsPerSubPartition * 4) / warps;
    if (regsPerWarp * roundUp(warps, 4) > d.regsPerBlock) {
        l.byRegs = 0;                             // the block alone is too big
    }
    const int smem = roundUp(smemPerBlock + d.reservedSmemPerBlock, 128);
    l.bySmem = smemPerBlock == 0 ? INT_MAX : d.smemPerSM / smem;
    l.byWarps = (d.maxThreadsPerSM / warpSize) / warps;
    l.byBlocks = d.maxBlocksPerSM;
    l.active = std::min({l.byRegs, l.bySmem, l.byWarps, l.byBlocks});
    if (l.active == l.byRegs) l.limiter += "registers ";
    if (l.active == l.bySmem) l.limiter += "shared-memory ";
    if (l.active == l.byWarps) l.limiter += "warps ";
    if (l.active == l.byBlocks) l.limiter += "blocks ";
    return l;
}

int headerModel(const Device& d, int threads, int regsPerThread, int smemPerBlock)
{
    cudaOccDeviceProp p;
    p.computeMajor = d.computeMajor;
    p.computeMinor = d.computeMinor;
    p.maxThreadsPerBlock = d.maxThreadsPerBlock;
    p.maxThreadsPerMultiprocessor = d.maxThreadsPerSM;
    p.regsPerBlock = d.regsPerBlock;
    p.regsPerMultiprocessor = d.regsPerSM;
    p.warpSize = 32;
    p.sharedMemPerBlock = 48 * 1024;
    p.sharedMemPerMultiprocessor = static_cast<size_t>(d.smemPerSM);
    p.numSms = 1;
    p.sharedMemPerBlockOptin = static_cast<size_t>(d.smemPerSM - d.reservedSmemPerBlock);
    p.reservedSharedMemPerBlock = static_cast<size_t>(d.reservedSmemPerBlock);
    cudaOccFuncAttributes a;
    a.maxThreadsPerBlock = d.maxThreadsPerBlock;
    a.numRegs = regsPerThread;
    a.sharedSizeBytes = static_cast<size_t>(smemPerBlock);
    cudaOccDeviceState s;
    cudaOccResult r;
    if (cudaOccMaxActiveBlocksPerMultiprocessor(&r, &p, &a, &s, threads, 0) != CUDA_OCC_SUCCESS) {
        return -1;
    }
    return r.activeBlocksPerMultiprocessor;
}

int main()
{
    // Example inputs (NOT a product specification; see the header comment).
    const Device d{8, 0, 2048, 32, 65536, 65536, 102400, 1024, 1024};
    std::printf("example device: %d threads/SM, %d blocks/SM, %d registers/SM, %d B shared/SM\n",
                d.maxThreadsPerSM, d.maxBlocksPerSM, d.regsPerSM, d.smemPerSM);
    std::printf("\n%-8s %-6s %-7s | %-5s %-5s %-5s %-6s | %-6s %-6s %-6s | %s\n", "threads", "regs",
                "smem B", "regs", "smem", "warps", "blocks", "active", "warps", "occup.", "limited by");
    struct Case { int threads, regs, smem; };
    const Case cases[] = {{256, 32, 0}, {256, 64, 0}, {256, 72, 0}, {256, 128, 0},
                          {128, 40, 8192}, {256, 32, 16384}, {256, 32, 49152}, {64, 32, 0},
                          {1024, 64, 0}, {1024, 65, 0}};
    for (const Case& c : cases) {
        const Limits l = ourModel(d, c.threads, c.regs, c.smem);
        const int warps = l.active * ((c.threads + 31) / 32);
        auto show = [](int v) { return v == INT_MAX ? std::string("-") : std::to_string(v); };
        std::printf("%-8d %-6d %-7d | %-5s %-5s %-5s %-6s | %-6d %-6d %5.1f%% | %s\n", c.threads, c.regs,
                    c.smem, show(l.byRegs).c_str(), show(l.bySmem).c_str(), show(l.byWarps).c_str(),
                    show(l.byBlocks).c_str(), l.active, warps, 100.0 * warps / (d.maxThreadsPerSM / 32),
                    l.limiter.c_str());
    }
    // Cross-check our model against NVIDIA's calculator over many inputs.
    int total = 0, agree = 0, shown = 0;
    for (int threads = 32; threads <= 1024; threads += 32) {
        for (int regs = 16; regs <= 255; regs += 3) {
            for (int smem = 0; smem <= 48 * 1024; smem += 2048) {
                ++total;
                const int ours = ourModel(d, threads, regs, smem).active;
                const int theirs = headerModel(d, threads, regs, smem);
                if (ours == theirs) {
                    ++agree;
                } else if (shown++ < 5) {
                    std::printf("differ: threads %d regs %d smem %d: ours %d, header %d\n", threads,
                                regs, smem, ours, theirs);
                }
            }
        }
    }
    std::printf("\ncross-check against cuda_occupancy.h: %d of %d input combinations agree\n", agree,
                total);
    return 0;
}
