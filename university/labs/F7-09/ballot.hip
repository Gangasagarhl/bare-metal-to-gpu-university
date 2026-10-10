// F7-09 Listing 3: one ballot per wave; the mask and its population count are stored.
// Complete program; the build container has no AMD GPU, so the run stops at the first call.
#include <cstdio>
#include <cstdlib>
#include <vector>
#include <hip/hip_runtime.h>

__global__ void countPositive(const float* x, unsigned long long* masks, int* counts)
{
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    unsigned long long m = __ballot(x[i] > 0.0f);   // 64-bit type on every target
    if (__lane_id() == 0) {
        masks[i / warpSize] = m;
        counts[i / warpSize] = __popcll(m);
    }
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
    const int n = 256;
    std::vector<float> hX(n);
    for (int i = 0; i < n; ++i) {
        hX[i] = (i % 3 == 0) ? 1.0f : -1.0f;      // every third element positive
    }
    float* dX = nullptr;
    unsigned long long* dMasks = nullptr;
    int* dCounts = nullptr;
    check(hipMalloc(&dX, n * sizeof(float)), "hipMalloc x");
    check(hipMalloc(&dMasks, n * sizeof(unsigned long long)), "hipMalloc masks");
    check(hipMalloc(&dCounts, n * sizeof(int)), "hipMalloc counts");
    check(hipMemcpy(dX, hX.data(), n * sizeof(float), hipMemcpyHostToDevice), "copy x");
    countPositive<<<1, n>>>(dX, dMasks, dCounts);
    check(hipGetLastError(), "kernel launch");
    std::vector<int> hCounts(n);
    check(hipMemcpy(hCounts.data(), dCounts, n * sizeof(int), hipMemcpyDeviceToHost), "copy counts");
    int wavesPerBlock = 0;
    hipDeviceProp_t prop{};
    check(hipGetDeviceProperties(&prop, 0), "hipGetDeviceProperties");
    wavesPerBlock = n / prop.warpSize;
    int total = 0;
    for (int w = 0; w < wavesPerBlock; ++w) {
        total += hCounts[w];
    }
    std::printf("warpSize %d, %d waves, %d positive (expected %d)\n", prop.warpSize, wavesPerBlock, total, (n + 2) / 3);
    check(hipFree(dX), "hipFree x");
    check(hipFree(dMasks), "hipFree masks");
    check(hipFree(dCounts), "hipFree counts");
    return total == (n + 2) / 3 ? 0 : 1;
}
