// F6-05 Listing 4: a kernel without its bounds check, for Compute Sanitizer.
// The grid has 1024 threads for 1000 elements: 24 threads write past the end.
#include <cstdio>
#include "cuda_check.h"

__global__ void fill(int* v, int value)
{
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    v[i] = value;                                   // no "if (i < n)": the bug
}

int main()
{
    const int n = 1000;
    int* d = nullptr;
    CUDA_CHECK(cudaMalloc(&d, n * sizeof(int)));
    fill<<<(n + 255) / 256, 256>>>(d, 7);
    CUDA_CHECK_LAUNCH();
    CUDA_CHECK(cudaDeviceSynchronize());
    CUDA_CHECK(cudaFree(d));
    std::printf("finished\n");
    return 0;
}
