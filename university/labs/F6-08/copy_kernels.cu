// F6-08 Listing 2: copy kernels for milestone E2, timed with the CU201 harness.
// copyStrided reads every stride-th element; copyOffset starts `offset` elements late.
#include <cstdio>
#include <vector>
#include "../F6-05/cuda_check.h"
#include "../F6-06/bench.h"

__global__ void copyStrided(const float* in, float* out, int n, int stride)
{
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    long long j = static_cast<long long>(i) * stride;   // 64-bit: i * stride can exceed int
    if (i < n) {
        out[i] = in[j];
    }
}

__global__ void copyOffset(const float* in, float* out, int n, int offset)
{
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i < n) {
        out[i] = in[i + offset];
    }
}

int main()
{
    const int n = 1 << 22;                         // elements copied by every variant
    const int maxStride = 32;
    const std::size_t inElems = static_cast<std::size_t>(n) * maxStride;
    float* dIn = nullptr;
    float* dOut = nullptr;
    CUDA_CHECK(cudaMalloc(&dIn, inElems * sizeof(float)));
    CUDA_CHECK(cudaMalloc(&dOut, n * sizeof(float)));
    CUDA_CHECK(cudaMemset(dIn, 0, inElems * sizeof(float)));
    cudaEvent_t start = nullptr;
    cudaEvent_t stop = nullptr;
    CUDA_CHECK(cudaEventCreate(&start));
    CUDA_CHECK(cudaEventCreate(&stop));
    const int threads = 256;
    const int blocks = (n + threads - 1) / threads;
    std::vector<bench::Row> rows;

    auto timeIt = [&](auto launch) {
        return [&, launch]() {
            CUDA_CHECK(cudaEventRecord(start));
            launch();
            CUDA_CHECK(cudaEventRecord(stop));
            CUDA_CHECK_LAUNCH();
            CUDA_CHECK(cudaEventSynchronize(stop));
            float ms = 0.0f;
            CUDA_CHECK(cudaEventElapsedTime(&ms, start, stop));
            return static_cast<double>(ms);
        };
    };
    for (int stride : {1, 2, 4, 8, 16, 32}) {
        auto f = timeIt([&, stride]() { copyStrided<<<blocks, threads>>>(dIn, dOut, n, stride); });
        rows.push_back({"cuda", "stride " + std::to_string(stride), n, 2.0 * n * sizeof(float),
                        bench::summarize(bench::measure(f, 3, 21))});
    }
    for (int offset : {0, 1, 2, 4, 8, 16, 32}) {
        auto f = timeIt([&, offset]() { copyOffset<<<blocks, threads>>>(dIn, dOut, n, offset); });
        rows.push_back({"cuda", "offset " + std::to_string(offset), n, 2.0 * n * sizeof(float),
                        bench::summarize(bench::measure(f, 3, 21))});
    }
    bench::printTable(rows);
    CUDA_CHECK(cudaEventDestroy(start));
    CUDA_CHECK(cudaEventDestroy(stop));
    CUDA_CHECK(cudaFree(dIn));
    CUDA_CHECK(cudaFree(dOut));
    return 0;
}
