// F7-20 Listing 1: one source for AMD (wave64 and wave32) and NVIDIA (warp32).
// Counts the positive elements of an array with a ballot per wave and one atomic per wave.
// Build (AMD, two targets in one binary):
//   hipcc -std=c++17 -O2 --offload-arch=gfx90a --offload-arch=gfx1100 portable.hip -o portable
// Build (NVIDIA through HIP's CUDA back end):
//   nvcc -std=c++17 -O2 -arch=sm_80 -x cu -D__HIP_PLATFORM_NVIDIA__ portable.hip -o portable_nv
#include <hip/hip_runtime.h>
#include <cstdio>
#include <cstdlib>
#include <vector>

static void check(hipError_t e, const char* what)
{
    if (e != hipSuccess) {
        std::printf("%s failed: %s (%s)\n", what, hipGetErrorName(e), hipGetErrorString(e));
        std::exit(1);
    }
}

// HIP 5.7's NVIDIA path maps __ballot to CUDA's old __ballot, which ptxas rejects for
// sm_70 and newer (run.sh step nv_ballot_v0); on NVIDIA we call __ballot_sync instead.
__device__ inline unsigned long long waveBallot(bool p)
{
#if defined(__HIP_PLATFORM_NVIDIA__)
    return __ballot_sync(0xFFFFFFFFu, p);           // all 32 lanes of the warp take part
#else
    return __ballot(p);                             // 64-bit mask; upper half 0 on wave32
#endif
}

__global__ void countPositive(const float* x, int n, unsigned long long* count)
{
    const int i = blockIdx.x * blockDim.x + threadIdx.x;
    const bool positive = (i < n) && (x[i] > 0.0f);
    // keep the mask 64-bit on every platform: a 32-bit variable loses lanes 32..63 on wave64
    const unsigned long long mask = waveBallot(positive);
    if (threadIdx.x % warpSize == 0) {              // first lane of each wave or warp
        atomicAdd(count, static_cast<unsigned long long>(__popcll(mask)));
    }
}

struct Tuning
{
    int block;          // threads per block: a whole number of waves on every target
    int maxBlocks;      // grid size cap, chosen per device after measuring
};

// Tuning is chosen on the HOST at run time from the device's properties. Device-side macros
// such as __gfx90a__ do not exist in the host pass, so the host cannot use them.
Tuning pickTuning(const hipDeviceProp_t& p)
{
    if (p.warpSize == 64) {
        return {256, 4 * p.multiProcessorCount};    // starting values: replace with measured ones
    }
    return {256, 8 * p.multiProcessorCount};
}

int main()
{
    hipDeviceProp_t prop;
    check(hipGetDeviceProperties(&prop, 0), "hipGetDeviceProperties");
    const Tuning t = pickTuning(prop);
    std::printf("device %s, warpSize %d, block %d\n", prop.name, prop.warpSize, t.block);
    const int n = 1000003;
    std::vector<float> h(n);
    unsigned long long expected = 0;
    for (int i = 0; i < n; ++i) {
        h[i] = static_cast<float>(static_cast<long long>(i) * 7919 % 201 - 100);
        expected += h[i] > 0.0f;
    }
    float* dx = nullptr;
    unsigned long long* dCount = nullptr;
    check(hipMalloc(&dx, n * sizeof(float)), "hipMalloc(dx)");
    check(hipMalloc(&dCount, sizeof(unsigned long long)), "hipMalloc(dCount)");
    check(hipMemcpy(dx, h.data(), n * sizeof(float), hipMemcpyHostToDevice), "copy x");
    check(hipMemset(dCount, 0, sizeof(unsigned long long)), "hipMemset");
    countPositive<<<(n + t.block - 1) / t.block, t.block>>>(dx, n, dCount);
    check(hipGetLastError(), "launch");
    unsigned long long got = 0;
    check(hipMemcpy(&got, dCount, sizeof(got), hipMemcpyDeviceToHost), "copy count");
    std::printf("count %llu, expected %llu\n", got, expected);
    check(hipFree(dx), "hipFree");
    check(hipFree(dCount), "hipFree");
    return got == expected ? 0 : 1;
}
