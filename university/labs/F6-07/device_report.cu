// F6-07 Listing 1: the harness's device report. One "key=value" line per fact, so that
// a program (Listing 2) can compare it with the vendor tool's report (milestone E1).
#include <cstdio>
#include "../F6-05/cuda_check.h"

int main()
{
    int runtime = 0;
    int driver = 0;
    CUDA_CHECK(cudaRuntimeGetVersion(&runtime));
    CUDA_CHECK(cudaDriverGetVersion(&driver));
    std::printf("runtime_version=%d\ndriver_version=%d\n", runtime, driver);

    int count = 0;
    CUDA_CHECK(cudaGetDeviceCount(&count));
    std::printf("device_count=%d\n", count);
    for (int d = 0; d < count; ++d) {
        cudaDeviceProp p{};
        CUDA_CHECK(cudaGetDeviceProperties(&p, d));
        std::printf("[device %d]\n", d);
        std::printf("name=%s\n", p.name);
        std::printf("compute_capability=%d.%d\n", p.major, p.minor);
        std::printf("sm_count=%d\n", p.multiProcessorCount);
        std::printf("warp_size=%d\n", p.warpSize);
        std::printf("memory_bytes=%zu\n", p.totalGlobalMem);
        std::printf("max_threads_per_block=%d\n", p.maxThreadsPerBlock);
        std::printf("max_block_dims=%d,%d,%d\n", p.maxThreadsDim[0], p.maxThreadsDim[1],
                    p.maxThreadsDim[2]);
        std::printf("max_grid_dims=%d,%d,%d\n", p.maxGridSize[0], p.maxGridSize[1],
                    p.maxGridSize[2]);
        std::printf("shared_mem_per_block=%zu\n", p.sharedMemPerBlock);
        std::printf("regs_per_block=%d\n", p.regsPerBlock);
        std::printf("const_mem_bytes=%zu\n", p.totalConstMem);
        std::printf("l2_cache_bytes=%d\n", p.l2CacheSize);
        std::printf("memory_bus_width_bits=%d\n", p.memoryBusWidth);
        std::printf("async_engine_count=%d\n", p.asyncEngineCount);
        std::printf("unified_addressing=%d\n", p.unifiedAddressing);
        std::printf("ecc_enabled=%d\n", p.ECCEnabled);
        std::printf("pci_bus_id=%d\n", p.pciBusID);
    }
    return 0;
}
