// F6-23 Listing 4: theoretical occupancy of every E6 step so far, from the compiler's own
// resource reports (resources.out of F6-21 to F6-24, sm_80) and NVIDIA's occupancy
// calculator header cuda_occupancy.h (host code, no GPU). The device is F6-20's EXAMPLE
// device, not a product: replace its numbers with what your GPU reports (F6-07).
#include <cuda_occupancy.h>
#include <cstdio>
#include <cstring>

struct Step
{
    const char* name;
    int threads, regs, smem;      // threads per block; registers per thread; static shared bytes
};

int main()
{
    cudaOccDeviceProp p;          // F6-20's example device
    p.computeMajor = 8;
    p.computeMinor = 0;
    p.maxThreadsPerBlock = 1024;
    p.maxThreadsPerMultiprocessor = 2048;
    p.regsPerBlock = 65536;
    p.regsPerMultiprocessor = 65536;
    p.warpSize = 32;
    p.sharedMemPerBlock = 48 * 1024;
    p.sharedMemPerMultiprocessor = 102400;
    p.numSms = 1;
    p.sharedMemPerBlockOptin = 102400 - 1024;
    p.reservedSharedMemPerBlock = 1024;
    const Step steps[] = {
        {"naive (F6-21)", 256, 32, 0},
        {"tiled16 (F6-22)", 256, 32, 2048},
        {"tiled32 (F6-22)", 1024, 32, 8192},
        {"reg1d 64x64/8x1", 512, 77, 4096},
        {"reg2d 64x64/4x4", 256, 75, 4096},
        {"vec 64x64/4x4", 256, 78, 4096},
        {"vec 128x128/8x8", 256, 124, 8192},
        {"vec 128x128/8x8 MINB=4", 256, 64, 8192},
        {"dbuf 64x64/4x4 (F6-24)", 256, 72, 8192},
        {"dbuf 128x128/8x8 (F6-24)", 256, 143, 16384},
    };
    std::printf("example device: %d threads/SM, %d registers/SM, %zu B shared/SM\n",
                p.maxThreadsPerMultiprocessor, p.regsPerMultiprocessor,
                p.sharedMemPerMultiprocessor);
    std::printf("%-26s %7s %5s %6s | %6s %6s %7s  %s\n", "step", "threads", "regs", "smem",
                "blocks", "warps", "occup.", "limited by");
    for (const Step& s : steps) {
        cudaOccFuncAttributes a;
        a.maxThreadsPerBlock = p.maxThreadsPerBlock;
        a.numRegs = s.regs;
        a.sharedSizeBytes = static_cast<size_t>(s.smem);
        cudaOccDeviceState st;
        cudaOccResult r;
        if (cudaOccMaxActiveBlocksPerMultiprocessor(&r, &p, &a, &st, s.threads, 0) !=
            CUDA_OCC_SUCCESS) {
            std::printf("%-26s calculator error\n", s.name);
            return 1;
        }
        const int warps = r.activeBlocksPerMultiprocessor * s.threads / 32;
        char why[64] = "";        // every factor that reaches the limit
        if (r.limitingFactors & OCC_LIMIT_REGISTERS) std::strcat(why, "registers ");
        if (r.limitingFactors & OCC_LIMIT_SHARED_MEMORY) std::strcat(why, "shared-memory ");
        if (r.limitingFactors & OCC_LIMIT_WARPS) std::strcat(why, "warps ");
        if (r.limitingFactors & OCC_LIMIT_BLOCKS) std::strcat(why, "blocks ");
        std::printf("%-26s %7d %5d %6d | %6d %6d %6.1f%%  %s\n", s.name, s.threads, s.regs, s.smem,
                    r.activeBlocksPerMultiprocessor, warps,
                    100.0 * warps / (p.maxThreadsPerMultiprocessor / 32), why);
    }
    // the largest register count per thread that still lets `want` blocks of 256 threads fit
    for (int want = 2; want <= 4; ++want) {
        int best = 0;
        for (int regs = 1; regs <= 255; ++regs) {
            cudaOccFuncAttributes a;
            a.maxThreadsPerBlock = p.maxThreadsPerBlock;
            a.numRegs = regs;
            a.sharedSizeBytes = 0;
            cudaOccDeviceState st;
            cudaOccResult r;
            if (cudaOccMaxActiveBlocksPerMultiprocessor(&r, &p, &a, &st, 256, 0) == CUDA_OCC_SUCCESS &&
                r.activeBlocksPerMultiprocessor >= want) {
                best = regs;
            }
        }
        std::printf("at most %3d registers per thread for %d resident blocks of 256 threads\n", best,
                    want);
    }
    return 0;
}
