// warp_sum.cu - a CUDA block sum written the way many CUDA codes are: 32 lanes assumed.
#include <hip/hip_runtime.h>
#include <cstdio>
#include <cstdlib>
#include <vector>

static void check(hipError_t err, const char* what)
{
    if (err != hipSuccess) {
        std::fprintf(stderr, "%s failed: %s\n", what, hipGetErrorString(err));
        std::exit(1);
    }
}

__global__ void blockSum(const int* in, int* total, int n)
{
    const int i = blockIdx.x * blockDim.x + threadIdx.x;
    int v = (i < n) ? in[i] : 0;
    for (int offset = 16; offset > 0; offset /= 2) {        // 32 lanes assumed
        v += __shfl_down_sync(0xffffffff, v, offset);        // 32-bit full mask
    }
    if (threadIdx.x % 32 == 0) {                             // one leader per 32 lanes
        atomicAdd(total, v);
    }
}

int main()
{
    const int n = 1 << 16;
    std::vector<int> hIn(n, 1);
    int* dIn = nullptr;
    int* dTotal = nullptr;
    check(hipMalloc(&dIn, n * sizeof(int)), "hipMalloc in");
    check(hipMalloc(&dTotal, sizeof(int)), "hipMalloc total");
    check(hipMemcpy(dIn, hIn.data(), n * sizeof(int), hipMemcpyHostToDevice), "copy in");
    check(hipMemset(dTotal, 0, sizeof(int)), "hipMemset");
    blockSum<<<n / 256, 256>>>(dIn, dTotal, n);
    check(hipGetLastError(), "launch blockSum");
    int hTotal = 0;
    check(hipMemcpy(&hTotal, dTotal, sizeof(int), hipMemcpyDeviceToHost), "copy total");
    std::printf("sum = %d (expected %d)\n", hTotal, n);
    check(hipFree(dIn), "hipFree in");
    check(hipFree(dTotal), "hipFree total");
    return hTotal == n ? 0 : 1;
}
