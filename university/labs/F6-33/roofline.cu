// F6-33 Listing 1: a family of kernels with rising arithmetic intensity, timed
// with events. poly<D> reads one float, does D multiply-adds, writes one float.
// The program prints achieved GFLOP/s and GB/s, the two coordinates of a roofline
// point. FLOP counts are the source's (2 per multiply-add); Listing 2 counts the
// real instructions in the SASS instead.
#include <cstdio>
#include "../F6-05/cuda_check.h"

__global__ void saxpy(int n, float a, const float* __restrict__ x, float* __restrict__ y)
{
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i < n) {
        y[i] = a * x[i] + y[i];
    }
}

template <int D>
__global__ void poly(int n, const float* __restrict__ x, float* __restrict__ y)
{
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i < n) {
        const float v = x[i];
        float acc = 1.0f;
#pragma unroll
        for (int k = 0; k < D; ++k) {
            acc = acc * v + 0.5f;                         // one multiply-add per step
        }
        y[i] = acc;
    }
}

template <class Launch>
float timeMs(Launch launch, cudaEvent_t start, cudaEvent_t stop)
{
    launch();                                             // warm-up
    CUDA_CHECK_LAUNCH();
    CUDA_CHECK(cudaEventRecord(start));
    const int runs = 21;
    for (int r = 0; r < runs; ++r) {
        launch();
    }
    CUDA_CHECK(cudaEventRecord(stop));
    CUDA_CHECK_LAUNCH();
    CUDA_CHECK(cudaEventSynchronize(stop));
    float ms = 0.0f;
    CUDA_CHECK(cudaEventElapsedTime(&ms, start, stop));
    return ms / runs;                                     // mean of 21 back-to-back runs
}

void report(const char* name, int n, double flopsPerElem, double bytesPerElem, float ms)
{
    const double s = ms * 1.0e-3;
    std::printf("%-10s AI %7.3f FLOP/B  %9.3f ms  %9.1f GFLOP/s  %8.1f GB/s\n", name,
                flopsPerElem / bytesPerElem, ms, flopsPerElem * n / s / 1e9,
                bytesPerElem * n / s / 1e9);
}

int main()
{
    const int n = 1 << 24;
    const std::size_t bytes = static_cast<std::size_t>(n) * sizeof(float);
    const int threads = 256;
    const int blocks = (n + threads - 1) / threads;
    float* dX = nullptr;
    float* dY = nullptr;
    CUDA_CHECK(cudaMalloc(&dX, bytes));
    CUDA_CHECK(cudaMalloc(&dY, bytes));
    CUDA_CHECK(cudaMemset(dX, 0, bytes));
    CUDA_CHECK(cudaMemset(dY, 0, bytes));
    cudaEvent_t start = nullptr;
    cudaEvent_t stop = nullptr;
    CUDA_CHECK(cudaEventCreate(&start));
    CUDA_CHECK(cudaEventCreate(&stop));

    auto run = [&](const char* name, double flops, double bytesPerElem, auto launch) {
        report(name, n, flops, bytesPerElem, timeMs(launch, start, stop));
    };
    run("saxpy", 2, 12, [&] { saxpy<<<blocks, threads>>>(n, 2.0f, dX, dY); });
    run("poly<1>", 2, 8, [&] { poly<1><<<blocks, threads>>>(n, dX, dY); });
    run("poly<4>", 8, 8, [&] { poly<4><<<blocks, threads>>>(n, dX, dY); });
    run("poly<16>", 32, 8, [&] { poly<16><<<blocks, threads>>>(n, dX, dY); });
    run("poly<64>", 128, 8, [&] { poly<64><<<blocks, threads>>>(n, dX, dY); });
    run("poly<256>", 512, 8, [&] { poly<256><<<blocks, threads>>>(n, dX, dY); });

    CUDA_CHECK(cudaEventDestroy(start));
    CUDA_CHECK(cudaEventDestroy(stop));
    CUDA_CHECK(cudaFree(dX));
    CUDA_CHECK(cudaFree(dY));
    return 0;
}
