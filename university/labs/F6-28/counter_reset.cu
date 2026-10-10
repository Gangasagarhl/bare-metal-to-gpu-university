// F6-28 forensic program: two jobs in two streams, plus one tiny kernel that
// resets a counter. Find the line that serialises the two streams.
#include <cstdio>
#include "../F6-05/cuda_check.h"

__global__ void work(float* data, int n, unsigned int* counter)
{
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i < n) {
        data[i] = data[i] * 2.0f + 1.0f;
        if (data[i] > 100.0f) {
            atomicAdd(counter, 1u);
        }
    }
}

__global__ void resetCounter(unsigned int* counter)
{
    *counter = 0u;
}

int main()
{
    const int n = 1 << 22;
    const std::size_t bytes = static_cast<std::size_t>(n) * sizeof(float);
    const int threads = 256;
    const int blocks = (n + threads - 1) / threads;
    float* hA = nullptr;
    float* hB = nullptr;
    CUDA_CHECK(cudaMallocHost(&hA, bytes));
    CUDA_CHECK(cudaMallocHost(&hB, bytes));
    for (int i = 0; i < n; ++i) {
        hA[i] = static_cast<float>(i % 200);
        hB[i] = static_cast<float>(i % 80);
    }
    float* dA = nullptr;
    float* dB = nullptr;
    unsigned int* dCounter = nullptr;
    CUDA_CHECK(cudaMalloc(&dA, bytes));
    CUDA_CHECK(cudaMalloc(&dB, bytes));
    CUDA_CHECK(cudaMalloc(&dCounter, sizeof(unsigned int)));
    cudaStream_t s1 = nullptr;
    cudaStream_t s2 = nullptr;
    CUDA_CHECK(cudaStreamCreate(&s1));
    CUDA_CHECK(cudaStreamCreate(&s2));

    CUDA_CHECK(cudaMemcpyAsync(dA, hA, bytes, cudaMemcpyHostToDevice, s1));
    work<<<blocks, threads, 0, s1>>>(dA, n, dCounter);
    CUDA_CHECK_LAUNCH();
    CUDA_CHECK(cudaMemcpyAsync(hA, dA, bytes, cudaMemcpyDeviceToHost, s1));

    resetCounter<<<1, 1>>>(dCounter);                     // no stream argument
    CUDA_CHECK_LAUNCH();

    CUDA_CHECK(cudaMemcpyAsync(dB, hB, bytes, cudaMemcpyHostToDevice, s2));
    work<<<blocks, threads, 0, s2>>>(dB, n, dCounter);
    CUDA_CHECK_LAUNCH();
    CUDA_CHECK(cudaMemcpyAsync(hB, dB, bytes, cudaMemcpyDeviceToHost, s2));
    CUDA_CHECK(cudaDeviceSynchronize());

    unsigned int count = 0;
    CUDA_CHECK(cudaMemcpy(&count, dCounter, sizeof(count), cudaMemcpyDeviceToHost));
    std::printf("job B: %u values above 100\n", count);

    CUDA_CHECK(cudaStreamDestroy(s1));
    CUDA_CHECK(cudaStreamDestroy(s2));
    CUDA_CHECK(cudaFree(dA));
    CUDA_CHECK(cudaFree(dB));
    CUDA_CHECK(cudaFree(dCounter));
    CUDA_CHECK(cudaFreeHost(hA));
    CUDA_CHECK(cudaFreeHost(hB));
    return 0;
}
