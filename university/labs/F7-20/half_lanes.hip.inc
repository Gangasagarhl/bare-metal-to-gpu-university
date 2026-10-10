// F7-20 forensic evidence: Priya's histogram kernel and its launch, ported unchanged from CUDA.
#include <hip/hip_runtime.h>

constexpr int kThreads = 32;   // "one warp per block keeps the code simple"

__global__ void __launch_bounds__(kThreads) histogram(const unsigned char* data, int n, unsigned int* bins)
{
    __shared__ unsigned int local[256];
    for (int b = threadIdx.x; b < 256; b += kThreads) {
        local[b] = 0;
    }
    __syncthreads();
    for (int i = blockIdx.x * kThreads + threadIdx.x; i < n; i += gridDim.x * kThreads) {
        atomicAdd(&local[data[i]], 1u);
    }
    __syncthreads();
    for (int b = threadIdx.x; b < 256; b += kThreads) {
        atomicAdd(&bins[b], local[b]);
    }
}

void launchHistogram(const unsigned char* d, int n, unsigned int* bins, int blocks)
{
    histogram<<<blocks, kThreads>>>(d, n, bins);
}
