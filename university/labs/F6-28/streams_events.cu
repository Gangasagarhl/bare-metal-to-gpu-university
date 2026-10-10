// F6-28 Listing 1: two streams, an event that joins them, and event timing.
// Stream A copies x in and scales it. Meanwhile stream B fills y. Stream B then
// waits for A's event, combines x and y, and copies the result out.
#include <cstdio>
#include <vector>
#include "../F6-05/cuda_check.h"

__global__ void scale(float* x, float a, int n)
{
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i < n) {
        x[i] = a * x[i];
    }
}

__global__ void fill(float* y, float value, int n)
{
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i < n) {
        y[i] = value;
    }
}

__global__ void combine(const float* x, float* y, int n)
{
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i < n) {
        y[i] = x[i] + y[i];
    }
}

int main()
{
    const int n = 1 << 22;
    const std::size_t bytes = static_cast<std::size_t>(n) * sizeof(float);
    const int threads = 256;
    const int blocks = (n + threads - 1) / threads;

    float* hX = nullptr;                                  // pinned: needed for async copies
    float* hOut = nullptr;
    CUDA_CHECK(cudaMallocHost(&hX, bytes));
    CUDA_CHECK(cudaMallocHost(&hOut, bytes));
    for (int i = 0; i < n; ++i) {
        hX[i] = static_cast<float>(i % 100);
    }
    float* dX = nullptr;
    float* dY = nullptr;
    CUDA_CHECK(cudaMalloc(&dX, bytes));
    CUDA_CHECK(cudaMalloc(&dY, bytes));

    cudaStream_t streamA = nullptr;
    cudaStream_t streamB = nullptr;
    CUDA_CHECK(cudaStreamCreateWithFlags(&streamA, cudaStreamNonBlocking));
    CUDA_CHECK(cudaStreamCreateWithFlags(&streamB, cudaStreamNonBlocking));
    cudaEvent_t xReady = nullptr;                         // a marker, not a stopwatch
    cudaEvent_t start = nullptr;
    cudaEvent_t stop = nullptr;
    CUDA_CHECK(cudaEventCreateWithFlags(&xReady, cudaEventDisableTiming));
    CUDA_CHECK(cudaEventCreate(&start));
    CUDA_CHECK(cudaEventCreate(&stop));

    CUDA_CHECK(cudaEventRecord(start, streamA));
    CUDA_CHECK(cudaMemcpyAsync(dX, hX, bytes, cudaMemcpyHostToDevice, streamA));
    scale<<<blocks, threads, 0, streamA>>>(dX, 2.0f, n);
    CUDA_CHECK_LAUNCH();
    CUDA_CHECK(cudaEventRecord(xReady, streamA));         // captures: copy in, scale

    fill<<<blocks, threads, 0, streamB>>>(dY, 1.0f, n);   // independent of stream A
    CUDA_CHECK_LAUNCH();
    CUDA_CHECK(cudaStreamWaitEvent(streamB, xReady, 0));  // later B work waits for A
    combine<<<blocks, threads, 0, streamB>>>(dX, dY, n);
    CUDA_CHECK_LAUNCH();
    CUDA_CHECK(cudaMemcpyAsync(hOut, dY, bytes, cudaMemcpyDeviceToHost, streamB));
    CUDA_CHECK(cudaEventRecord(stop, streamB));

    CUDA_CHECK(cudaEventSynchronize(stop));               // host waits for B's work only
    float ms = 0.0f;
    CUDA_CHECK(cudaEventElapsedTime(&ms, start, stop));

    long long errors = 0;
    for (int i = 0; i < n; ++i) {
        const float expected = 2.0f * hX[i] + 1.0f;       // small integers: exact in float
        if (hOut[i] != expected) {
            ++errors;
        }
    }
    std::printf("checked %d elements, %lld errors, start->stop %.3f ms\n", n, errors, ms);

    CUDA_CHECK(cudaEventDestroy(xReady));
    CUDA_CHECK(cudaEventDestroy(start));
    CUDA_CHECK(cudaEventDestroy(stop));
    CUDA_CHECK(cudaStreamDestroy(streamA));
    CUDA_CHECK(cudaStreamDestroy(streamB));
    CUDA_CHECK(cudaFree(dX));
    CUDA_CHECK(cudaFree(dY));
    CUDA_CHECK(cudaFreeHost(hX));
    CUDA_CHECK(cudaFreeHost(hOut));
    return errors == 0 ? 0 : 1;
}
