// F1-62 Listing 2: what the CUDA runtime can tell you about links between GPUs
// and about page-locked host memory. Untested on hardware (no GPU in the build).
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
    for (int d = 0; d < count; ++d) {
        cudaDeviceProp p{};
        check(cudaGetDeviceProperties(&p, d), "cudaGetDeviceProperties");
        std::printf("device %d: %s, PCI bus %d, asyncEngineCount %d\n", d, p.name, p.pciBusID, p.asyncEngineCount);
    }
    for (int a = 0; a < count; ++a) {
        for (int b = 0; b < count; ++b) {
            if (a == b) { continue; }
            int can = 0;
            check(cudaDeviceCanAccessPeer(&can, a, b), "cudaDeviceCanAccessPeer");
            std::printf("device %d can access device %d directly: %s\n", a, b, can ? "yes" : "no");
        }
    }
    void* pinned = nullptr;                      // page-locked host buffer for fast, async copies
    check(cudaMallocHost(&pinned, 1 << 20), "cudaMallocHost");
    check(cudaFreeHost(pinned), "cudaFreeHost");
    std::printf("page-locked allocation of 1 MiB: ok\n");
    return 0;
}
