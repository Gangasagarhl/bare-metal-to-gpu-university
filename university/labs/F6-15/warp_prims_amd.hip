// F6-15 Listing 3: the warp reduction and compaction in HIP for AMD GPUs.
// No hard-coded 32: the loop starts at warpSize / 2 and the ballot mask is 64 bits wide.
// Built for real for gfx90a (wavefront size 64); untested on hardware.
#include <cstdio>
#include <cstdlib>
#include <vector>
#include <hip/hip_runtime.h>

__device__ int waveReduce(int v)
{
    for (int off = warpSize / 2; off > 0; off /= 2) { v += __shfl_down(v, off); }
    return v;                                            // valid in lane 0
}

__global__ void waveTests(const int* in, int* sums, int* keptCount)
{
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    int lane = threadIdx.x % warpSize;
    int wave = i / warpSize;
    int v = in[i];
    int s = waveReduce(v);
    unsigned long long mask = __ballot(v % 3 == 0);      // 64-bit: one bit per lane
    if (lane == 0) {
        sums[wave] = s;
        keptCount[wave] = __popcll(mask);
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
    int dev = 0;
    check(hipGetDevice(&dev), "hipGetDevice");
    hipDeviceProp_t prop;
    check(hipGetDeviceProperties(&prop, dev), "hipGetDeviceProperties");
    const int width = prop.warpSize;                     // ask the device; do not assume
    const int n = 1 << 20, threads = 256, waves = n / width;
    std::vector<int> h(n);
    for (int i = 0; i < n; ++i) { h[i] = (i * 37) % 101; }
    int *dIn, *dSums, *dCount;
    check(hipMalloc(&dIn, sizeof(int) * n), "malloc");
    check(hipMalloc(&dSums, sizeof(int) * waves), "malloc");
    check(hipMalloc(&dCount, sizeof(int) * waves), "malloc");
    check(hipMemcpy(dIn, h.data(), sizeof(int) * n, hipMemcpyHostToDevice), "copy in");
    waveTests<<<n / threads, threads>>>(dIn, dSums, dCount);
    check(hipGetLastError(), "waveTests");
    std::vector<int> sums(waves), count(waves);
    check(hipMemcpy(sums.data(), dSums, sizeof(int) * waves, hipMemcpyDeviceToHost), "copy");
    check(hipMemcpy(count.data(), dCount, sizeof(int) * waves, hipMemcpyDeviceToHost), "copy");
    int errors = 0;
    for (int w = 0; w < waves; ++w) {
        int s = 0, k = 0;
        for (int lane = 0; lane < width; ++lane) { s += h[w * width + lane]; k += h[w * width + lane] % 3 == 0; }
        errors += (sums[w] != s) + (count[w] != k);
    }
    std::printf("waveTests: wavefront size %d, %d wavefronts checked, %d errors\n", width, waves, errors);
    check(hipFree(dIn), "free");
    check(hipFree(dSums), "free");
    check(hipFree(dCount), "free");
    return 0;
}
