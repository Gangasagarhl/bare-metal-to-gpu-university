// F8-04 Listing 3: the attribute part of Listing 2 in HIP, for AMD GPUs.
// Prints peer access, the link performance rank and native atomics for every GPU pair.
// Built for real with hipcc; untested on hardware (the build container has no GPU).
// Note: the HIP header numbers hipDevP2PAttrPerformanceRank 0, the CUDA header numbers
// cudaDevP2PAttrPerformanceRank 1. Always use the names, never the numbers.
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
    int n = 0;
    check(hipGetDeviceCount(&n), "hipGetDeviceCount");
    std::printf("%d GPU(s)\npair   canAccess  perfRank  atomics\n", n);
    for (int a = 0; a < n; ++a) {
        for (int b = 0; b < n; ++b) {
            if (a == b) {
                continue;
            }
            int can = 0, rank = 0, atomics = 0;
            check(hipDeviceCanAccessPeer(&can, a, b), "hipDeviceCanAccessPeer");
            check(hipDeviceGetP2PAttribute(&rank, hipDevP2PAttrPerformanceRank, a, b), "P2P perf rank");
            check(hipDeviceGetP2PAttribute(&atomics, hipDevP2PAttrNativeAtomicSupported, a, b), "P2P atomics");
            std::printf("%d->%d  %9d %9d %8d\n", a, b, can, rank, atomics);
        }
    }
    return 0;
}
