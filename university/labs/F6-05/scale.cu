// F6-05 Listing 2: a small program that checks every call with cuda_check.h.
#include <cstdio>
#include <vector>
#include "cuda_check.h"

__global__ void scale(float* v, int n, float factor)
{
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i < n) {
        v[i] *= factor;
    }
}

int main()
{
    const int n = 1000;
    std::vector<float> h(n, 2.0f);
    float* d = nullptr;
    CUDA_CHECK(cudaMalloc(&d, n * sizeof(float)));
    CUDA_CHECK(cudaMemcpy(d, h.data(), n * sizeof(float), cudaMemcpyHostToDevice));
    scale<<<(n + 255) / 256, 256>>>(d, n, 3.0f);
    CUDA_CHECK_LAUNCH();
    CUDA_CHECK(cudaMemcpy(h.data(), d, n * sizeof(float), cudaMemcpyDeviceToHost));
    CUDA_CHECK(cudaFree(d));
    std::printf("h[0] = %.1f, h[%d] = %.1f\n", h[0], n - 1, h[n - 1]);
    return h[0] == 6.0f && h[n - 1] == 6.0f ? 0 : 1;
}
