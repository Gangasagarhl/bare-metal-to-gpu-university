// F1-56 Listing 1: our own small "deviceQuery" for NVIDIA GPUs.
// Every field name below is a member of cudaDeviceProp in the installed CUDA 12.0
// header driver_types.h; the comment after each printf is that header's own comment.
#include <cstdio>
#include <cstdlib>
#include <cuda_runtime.h>

static void check(cudaError_t err, const char* what)
{
    if (err != cudaSuccess) {
        std::fprintf(stderr, "%s failed: %s (%s)\n", what, cudaGetErrorName(err), cudaGetErrorString(err));
        std::exit(EXIT_FAILURE);
    }
}

int main()
{
    int count = 0;
    check(cudaGetDeviceCount(&count), "cudaGetDeviceCount");
    std::printf("CUDA devices: %d\n", count);
    for (int d = 0; d < count; ++d) {
        cudaDeviceProp p{};
        check(cudaGetDeviceProperties(&p, d), "cudaGetDeviceProperties");
        std::printf("device %d: %s\n", d, p.name);
        std::printf("  compute capability          %d.%d\n", p.major, p.minor);
        std::printf("  multiProcessorCount         %d\n", p.multiProcessorCount);   // Number of multiprocessors on device
        std::printf("  warpSize                    %d\n", p.warpSize);              // Warp size in threads
        std::printf("  maxThreadsPerMultiProcessor %d\n", p.maxThreadsPerMultiProcessor);
        std::printf("  maxBlocksPerMultiProcessor  %d\n", p.maxBlocksPerMultiProcessor);
        std::printf("  regsPerMultiprocessor       %d\n", p.regsPerMultiprocessor); // 32-bit registers
        std::printf("  sharedMemPerMultiprocessor  %zu bytes\n", p.sharedMemPerMultiprocessor);
        std::printf("  l2CacheSize                 %d bytes\n", p.l2CacheSize);
        std::printf("  totalGlobalMem              %zu bytes\n", p.totalGlobalMem);
        std::printf("  memoryBusWidth              %d bits\n", p.memoryBusWidth);
        std::printf("  pciBusID                    %d\n", p.pciBusID);
    }
    return 0;
}
