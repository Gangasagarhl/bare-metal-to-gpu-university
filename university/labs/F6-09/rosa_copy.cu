// F6-09 forensic evidence: Rosa's timing loop after her "switch to vector loads"
// (deliberately broken; do not copy). She changed the grid for float4 but kept
// launching the scalar kernel, and her timing runs skip the correctness check.
#include <cstdio>
#include "../F6-05/cuda_check.h"
#include "../F6-06/bench.h"

__global__ void copyScalar(const float* in, float* out, int n)
{
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i < n) {
        out[i] = in[i];
    }
}

int main()
{
    const int n = 1 << 26;
    const std::size_t bytes = static_cast<std::size_t>(n) * sizeof(float);
    float* dIn = nullptr;
    float* dOut = nullptr;
    CUDA_CHECK(cudaMalloc(&dIn, bytes));
    CUDA_CHECK(cudaMalloc(&dOut, bytes));
    cudaEvent_t start = nullptr;
    cudaEvent_t stop = nullptr;
    CUDA_CHECK(cudaEventCreate(&start));
    CUDA_CHECK(cudaEventCreate(&stop));
    const int threads = 256;
    const int n4 = n / 4;                                   // new grid "for float4"
    auto once = [&]() {
        CUDA_CHECK(cudaEventRecord(start));
        copyScalar<<<(n4 + threads - 1) / threads, threads>>>(dIn, dOut, n);  // the bug
        CUDA_CHECK(cudaEventRecord(stop));
        CUDA_CHECK_LAUNCH();
        CUDA_CHECK(cudaEventSynchronize(stop));
        float ms = 0.0f;
        CUDA_CHECK(cudaEventElapsedTime(&ms, start, stop));
        return static_cast<double>(ms);
    };
    bench::Row row{"cuda", "copy (rosa)", n, 2.0 * bytes, bench::summarize(bench::measure(once, 3, 21))};
    bench::printTable({row});
    CUDA_CHECK(cudaFree(dIn));
    CUDA_CHECK(cudaFree(dOut));
    return 0;
}
