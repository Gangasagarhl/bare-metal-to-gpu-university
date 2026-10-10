// F6-06 Listing 3: timing a kernel correctly with CUDA events, and the wrong way
// with a host clock and no synchronisation (for comparison). Uses the harness.
#include <chrono>
#include <cstdio>
#include <vector>
#include "../F6-05/cuda_check.h"
#include "bench.h"

__global__ void saxpy(int n, float a, const float* x, float* y)
{
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i < n) {
        y[i] = a * x[i] + y[i];
    }
}

int main()
{
    const int n = 1 << 24;
    const std::size_t bytes = static_cast<std::size_t>(n) * sizeof(float);
    std::vector<float> hX(n, 1.0f), hY(n, 2.0f);
    float* dX = nullptr;
    float* dY = nullptr;
    CUDA_CHECK(cudaMalloc(&dX, bytes));
    CUDA_CHECK(cudaMalloc(&dY, bytes));
    CUDA_CHECK(cudaMemcpy(dX, hX.data(), bytes, cudaMemcpyHostToDevice));
    CUDA_CHECK(cudaMemcpy(dY, hY.data(), bytes, cudaMemcpyHostToDevice));
    const int threads = 256;
    const int blocks = (n + threads - 1) / threads;

    // Wrong: the launch returns before the kernel finishes; this times the launch only.
    const auto t0 = std::chrono::steady_clock::now();
    saxpy<<<blocks, threads>>>(n, 0.5f, dX, dY);
    const auto t1 = std::chrono::steady_clock::now();
    CUDA_CHECK_LAUNCH();
    CUDA_CHECK(cudaDeviceSynchronize());
    std::printf("host clock, no sync: %.4f ms (launch only)\n",
                std::chrono::duration<double, std::milli>(t1 - t0).count());

    // Right: events recorded in the same stream around the kernel.
    cudaEvent_t start = nullptr;
    cudaEvent_t stop = nullptr;
    CUDA_CHECK(cudaEventCreate(&start));
    CUDA_CHECK(cudaEventCreate(&stop));
    auto timeOnce = [&]() {
        CUDA_CHECK(cudaEventRecord(start));
        saxpy<<<blocks, threads>>>(n, 0.5f, dX, dY);
        CUDA_CHECK(cudaEventRecord(stop));
        CUDA_CHECK_LAUNCH();
        CUDA_CHECK(cudaEventSynchronize(stop));      // wait until stop has happened
        float ms = 0.0f;
        CUDA_CHECK(cudaEventElapsedTime(&ms, start, stop));
        return static_cast<double>(ms);
    };
    const std::vector<double> runs = bench::measure(timeOnce, 3, 21);
    bench::Row row{"cuda", "saxpy", n, 3.0 * bytes, bench::summarize(runs)};
    bench::printTable({row});

    CUDA_CHECK(cudaEventDestroy(start));
    CUDA_CHECK(cudaEventDestroy(stop));
    CUDA_CHECK(cudaFree(dX));
    CUDA_CHECK(cudaFree(dY));
    return 0;
}
