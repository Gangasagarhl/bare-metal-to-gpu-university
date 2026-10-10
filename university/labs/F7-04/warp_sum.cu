// warp_sum.cu - a CUDA block sum written the way many CUDA codes are: 32 lanes assumed.
#include <cuda_runtime.h>
#include <cstdio>
#include <cstdlib>
#include <vector>

static void check(cudaError_t err, const char* what)
{
    if (err != cudaSuccess) {
        std::fprintf(stderr, "%s failed: %s\n", what, cudaGetErrorString(err));
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
    check(cudaMalloc(&dIn, n * sizeof(int)), "cudaMalloc in");
    check(cudaMalloc(&dTotal, sizeof(int)), "cudaMalloc total");
    check(cudaMemcpy(dIn, hIn.data(), n * sizeof(int), cudaMemcpyHostToDevice), "copy in");
    check(cudaMemset(dTotal, 0, sizeof(int)), "cudaMemset");
    blockSum<<<n / 256, 256>>>(dIn, dTotal, n);
    check(cudaGetLastError(), "launch blockSum");
    int hTotal = 0;
    check(cudaMemcpy(&hTotal, dTotal, sizeof(int), cudaMemcpyDeviceToHost), "copy total");
    std::printf("sum = %d (expected %d)\n", hTotal, n);
    check(cudaFree(dIn), "cudaFree in");
    check(cudaFree(dTotal), "cudaFree total");
    return hTotal == n ? 0 : 1;
}
