// F7-08 Listing 2: every work-item records the wave width, its wave and its lane.
// Complete program; the build container has no AMD GPU, so the run stops at the first call.
#include <cstdio>
#include <cstdlib>
#include <vector>
#include <hip/hip_runtime.h>

__global__ void waveInfo(int* out)
{
    int t = threadIdx.x;
    out[3 * t + 0] = warpSize;          // compile-time constant of the target
    out[3 * t + 1] = t / warpSize;      // which wave of the workgroup
    out[3 * t + 2] = __lane_id();       // position inside the wave
}

static void check(hipError_t err, const char* what)
{
    if (err != hipSuccess) {
        std::fprintf(stderr, "%s failed: %s (%s)\n", what, hipGetErrorName(err), hipGetErrorString(err));
        std::exit(EXIT_FAILURE);
    }
}

int main()
{
    hipDeviceProp_t prop{};
    check(hipGetDeviceProperties(&prop, 0), "hipGetDeviceProperties");
    std::printf("device %s: warpSize %d, compute units %d\n",
                prop.gcnArchName, prop.warpSize, prop.multiProcessorCount);

    const int threads = 256;
    std::vector<int> hOut(3 * threads);
    int* dOut = nullptr;
    check(hipMalloc(&dOut, hOut.size() * sizeof(int)), "hipMalloc");
    waveInfo<<<1, threads>>>(dOut);
    check(hipGetLastError(), "kernel launch");
    check(hipMemcpy(hOut.data(), dOut, hOut.size() * sizeof(int), hipMemcpyDeviceToHost), "hipMemcpy");
    for (int t : {0, 31, 32, 63, 64, 255}) {
        std::printf("thread %3d: warpSize %d wave %d lane %d\n", t, hOut[3 * t], hOut[3 * t + 1], hOut[3 * t + 2]);
    }
    check(hipFree(dOut), "hipFree");
    return 0;
}
