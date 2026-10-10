// F6-33 forensic program: two ways to divide every element by the same number.
// divideEach uses a division per element; multiplyInverse multiplies by 1/s,
// computed once on the host (results may differ in the last bit; see the chapter).
#include <cstdio>
#include <vector>
#include "../F6-05/cuda_check.h"

__global__ void divideEach(int n, float s, const float* __restrict__ x, float* __restrict__ y)
{
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i < n) {
        y[i] = x[i] / s;
    }
}

__global__ void multiplyInverse(int n, float inv, const float* __restrict__ x,
                                float* __restrict__ y)
{
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i < n) {
        y[i] = x[i] * inv;
    }
}

int main()
{
    const int n = 1 << 24;
    const std::size_t bytes = static_cast<std::size_t>(n) * sizeof(float);
    const float s = 3.0f;
    const int threads = 256;
    const int blocks = (n + threads - 1) / threads;
    std::vector<float> h(static_cast<std::size_t>(n));
    for (int i = 0; i < n; ++i) {
        h[static_cast<std::size_t>(i)] = static_cast<float>(i % 1000);
    }
    float* dX = nullptr;
    float* dY = nullptr;
    CUDA_CHECK(cudaMalloc(&dX, bytes));
    CUDA_CHECK(cudaMalloc(&dY, bytes));
    CUDA_CHECK(cudaMemcpy(dX, h.data(), bytes, cudaMemcpyHostToDevice));
    cudaEvent_t e0 = nullptr;
    cudaEvent_t e1 = nullptr;
    cudaEvent_t e2 = nullptr;
    CUDA_CHECK(cudaEventCreate(&e0));
    CUDA_CHECK(cudaEventCreate(&e1));
    CUDA_CHECK(cudaEventCreate(&e2));
    divideEach<<<blocks, threads>>>(n, s, dX, dY);        // warm-up both
    multiplyInverse<<<blocks, threads>>>(n, 1.0f / s, dX, dY);
    CUDA_CHECK_LAUNCH();
    CUDA_CHECK(cudaEventRecord(e0));
    divideEach<<<blocks, threads>>>(n, s, dX, dY);
    CUDA_CHECK(cudaEventRecord(e1));
    multiplyInverse<<<blocks, threads>>>(n, 1.0f / s, dX, dY);
    CUDA_CHECK(cudaEventRecord(e2));
    CUDA_CHECK_LAUNCH();
    CUDA_CHECK(cudaEventSynchronize(e2));
    float divMs = 0.0f;
    float mulMs = 0.0f;
    CUDA_CHECK(cudaEventElapsedTime(&divMs, e0, e1));
    CUDA_CHECK(cudaEventElapsedTime(&mulMs, e1, e2));
    std::printf("divideEach %.3f ms, multiplyInverse %.3f ms\n", divMs, mulMs);
    CUDA_CHECK(cudaEventDestroy(e0));
    CUDA_CHECK(cudaEventDestroy(e1));
    CUDA_CHECK(cudaEventDestroy(e2));
    CUDA_CHECK(cudaFree(dX));
    CUDA_CHECK(cudaFree(dY));
    return 0;
}
