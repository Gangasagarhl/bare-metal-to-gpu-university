// wave_sum.hip - a block sum ported from CUDA. Every lane also reports what it holds
// after the shuffle loop, so a wrong total can be traced lane by lane.
#include <hip/hip_runtime.h>
#include <cstdio>
#include <cstdlib>
#include <vector>

static void check(hipError_t err, const char* what)
{
    if (err != hipSuccess) {
        std::fprintf(stderr, "%s failed: %s (%s)\n", what, hipGetErrorName(err),
                     hipGetErrorString(err));
        std::exit(1);
    }
}

__global__ void waveSum(const int* in, int* lanes, int* total, int n)
{
    const int i = blockIdx.x * blockDim.x + threadIdx.x;
    int v = (i < n) ? in[i] : 0;
    for (int offset = 16; offset > 0; offset /= 2) {       // butterfly over 32 lanes
        v += __shfl_xor(v, offset);
    }
    if (i < n) {
        lanes[i] = v;                                       // per-lane evidence
    }
    if (threadIdx.x % warpSize == 0) {                      // one leader per wavefront
        atomicAdd(total, v);
    }
}

int main()
{
    const int n = 64;                                       // one block of 64 threads
    std::vector<int> hIn(n);
    int expected = 0;
    for (int i = 0; i < n; ++i) {
        hIn[i] = i + 1;
        expected += i + 1;
    }
    int* dIn = nullptr;
    int* dLanes = nullptr;
    int* dTotal = nullptr;
    check(hipMalloc(&dIn, n * sizeof(int)), "hipMalloc in");
    check(hipMalloc(&dLanes, n * sizeof(int)), "hipMalloc lanes");
    check(hipMalloc(&dTotal, sizeof(int)), "hipMalloc total");
    check(hipMemcpy(dIn, hIn.data(), n * sizeof(int), hipMemcpyHostToDevice), "copy in");
    check(hipMemset(dTotal, 0, sizeof(int)), "hipMemset total");
    waveSum<<<1, n>>>(dIn, dLanes, dTotal, n);
    check(hipGetLastError(), "launch waveSum");
    std::vector<int> hLanes(n);
    int hTotal = 0;
    check(hipMemcpy(hLanes.data(), dLanes, n * sizeof(int), hipMemcpyDeviceToHost), "copy lanes");
    check(hipMemcpy(&hTotal, dTotal, sizeof(int), hipMemcpyDeviceToHost), "copy total");
    for (int i = 0; i < n; ++i) {
        std::printf("%2d:%5d%s", i, hLanes[i], (i % 8 == 7) ? "\n" : "   ");
    }
    std::printf("total = %d (expected %d)\n", hTotal, expected);
    check(hipFree(dIn), "hipFree in");
    check(hipFree(dLanes), "hipFree lanes");
    check(hipFree(dTotal), "hipFree total");
    return hTotal == expected ? 0 : 1;
}
