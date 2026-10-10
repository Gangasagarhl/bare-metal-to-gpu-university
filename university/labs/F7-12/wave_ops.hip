// F7-12 Listing 2: portable wave-level operations in HIP: a sum with __shfl_down and
// a stream compaction with __ballot. Nothing assumes 32 or 64; warpSize decides.
// Complete program; the build container has no AMD GPU, so the run stops at the first call.
#include <cstdio>
#include <cstdlib>
#include <vector>
#include <hip/hip_runtime.h>

__device__ int waveSum(int v)
{
    for (int d = warpSize / 2; d > 0; d /= 2) {
        v += __shfl_down(v, d);
    }
    return v;                                        // valid in lane 0
}

// Each wave keeps its positive values, packed, in its own segment of `out`.
__global__ void sumAndCompact(const int* in, int* waveSums, int* out, int* outCount)
{
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    int wave = i / warpSize;
    int lane = __lane_id();
    int s = waveSum(in[i]);
    unsigned long long keep = __ballot(in[i] > 0);
    unsigned long long below = keep & ((1ull << lane) - 1ull);   // lanes under me that keep
    if (in[i] > 0) {
        out[wave * warpSize + __popcll(below)] = in[i];
    }
    if (lane == 0) {
        waveSums[wave] = s;
        outCount[wave] = __popcll(keep);
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
    hipDeviceProp_t prop{};
    check(hipGetDeviceProperties(&prop, 0), "hipGetDeviceProperties");
    const int n = 256;
    const int waves = n / prop.warpSize;
    std::vector<int> hIn(n);
    for (int i = 0; i < n; ++i) {
        hIn[i] = (i % 3 == 0) ? i : -i;
    }
    int *dIn = nullptr, *dSums = nullptr, *dOut = nullptr, *dCount = nullptr;
    check(hipMalloc(&dIn, n * sizeof(int)), "hipMalloc in");
    check(hipMalloc(&dSums, waves * sizeof(int)), "hipMalloc sums");
    check(hipMalloc(&dOut, n * sizeof(int)), "hipMalloc out");
    check(hipMalloc(&dCount, waves * sizeof(int)), "hipMalloc count");
    check(hipMemcpy(dIn, hIn.data(), n * sizeof(int), hipMemcpyHostToDevice), "copy in");
    sumAndCompact<<<1, n>>>(dIn, dSums, dOut, dCount);
    check(hipGetLastError(), "kernel launch");
    std::vector<int> hSums(waves), hCount(waves);
    check(hipMemcpy(hSums.data(), dSums, waves * sizeof(int), hipMemcpyDeviceToHost), "copy sums");
    check(hipMemcpy(hCount.data(), dCount, waves * sizeof(int), hipMemcpyDeviceToHost), "copy count");
    int errors = 0;
    for (int w = 0; w < waves; ++w) {
        int sum = 0, count = 0;
        for (int l = 0; l < prop.warpSize; ++l) {
            sum += hIn[w * prop.warpSize + l];
            count += hIn[w * prop.warpSize + l] > 0;
        }
        errors += (sum != hSums[w]) + (count != hCount[w]);
    }
    std::printf("warpSize %d, %d waves, %d errors\n", prop.warpSize, waves, errors);
    check(hipFree(dIn), "hipFree in");
    check(hipFree(dSums), "hipFree sums");
    check(hipFree(dOut), "hipFree out");
    check(hipFree(dCount), "hipFree count");
    return errors == 0 ? 0 : 1;
}
