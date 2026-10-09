// F1-56 Listing 2 (device_query_amd.hip): the same query for AMD GPUs through HIP.
// Field names are members of hipDeviceProp_t in the installed HIP 5.7 header
// hip_runtime_api.h; that header calls multiProcessorCount "compute units".
#include <cstdio>
#include <cstdlib>
#include <hip/hip_runtime.h>

static void check(hipError_t err, const char* what)
{
    if (err != hipSuccess) {
        std::fprintf(stderr, "%s failed: %s (%s)\n", what, hipGetErrorName(err), hipGetErrorString(err));
        std::exit(EXIT_FAILURE);
    }
}

int main()
{
    int count = 0;
    check(hipGetDeviceCount(&count), "hipGetDeviceCount");
    std::printf("HIP devices: %d\n", count);
    for (int d = 0; d < count; ++d) {
        hipDeviceProp_t p{};
        check(hipGetDeviceProperties(&p, d), "hipGetDeviceProperties");
        std::printf("device %d: %s (%s)\n", d, p.name, p.gcnArchName);
        std::printf("  multiProcessorCount (CUs)        %d\n", p.multiProcessorCount);
        std::printf("  warpSize (wavefront size)        %d\n", p.warpSize);
        std::printf("  maxThreadsPerMultiProcessor      %d\n", p.maxThreadsPerMultiProcessor);
        std::printf("  maxSharedMemoryPerMultiProcessor %zu bytes\n", p.maxSharedMemoryPerMultiProcessor);
        std::printf("  l2CacheSize                      %d bytes\n", p.l2CacheSize);
        std::printf("  totalGlobalMem                   %zu bytes\n", p.totalGlobalMem);
        std::printf("  memoryBusWidth                   %d bits\n", p.memoryBusWidth);
    }
    return 0;
}
