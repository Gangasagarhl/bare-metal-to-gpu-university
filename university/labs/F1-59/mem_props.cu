// F1-59 Listing 3: what the CUDA runtime reports about a GPU's memory system.
// Field names and their meanings are from the comments in the installed CUDA 12.0
// header driver_types.h (cudaDeviceProp). Untested on hardware.
#include <cstdio>
#include <cstdlib>
#include <cuda_runtime.h>

int main()
{
    int count = 0;
    cudaError_t err = cudaGetDeviceCount(&count);
    if (err != cudaSuccess) {
        std::fprintf(stderr, "cudaGetDeviceCount failed: %s (%s)\n", cudaGetErrorName(err), cudaGetErrorString(err));
        return EXIT_FAILURE;
    }
    for (int d = 0; d < count; ++d) {
        cudaDeviceProp p{};
        err = cudaGetDeviceProperties(&p, d);
        if (err != cudaSuccess) {
            std::fprintf(stderr, "cudaGetDeviceProperties failed: %s\n", cudaGetErrorString(err));
            return EXIT_FAILURE;
        }
        std::printf("device %d: %s\n", d, p.name);
        std::printf("  totalGlobalMem   %zu bytes\n", p.totalGlobalMem);   // Global memory available on device in bytes
        std::printf("  memoryBusWidth   %d bits\n", p.memoryBusWidth);     // Global memory bus width in bits
        std::printf("  memoryClockRate  %d kHz\n", p.memoryClockRate);     // Deprecated, Peak memory clock frequency in kilohertz
        std::printf("  l2CacheSize      %d bytes\n", p.l2CacheSize);       // Size of L2 cache in bytes
        std::printf("  ECCEnabled       %d\n", p.ECCEnabled);              // Device has ECC support enabled
    }
    return 0;
}
