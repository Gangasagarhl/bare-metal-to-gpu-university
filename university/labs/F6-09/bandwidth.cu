// F6-09 Listing 2: milestone E2 measurements with the CU201 harness.
// Part 1: device copy bandwidth, scalar loads versus 16-byte vector loads.
// Part 2: host-to-device and device-to-host copies from pageable and pinned memory.
#include <cstdio>
#include <vector>
#include "../F6-05/cuda_check.h"
#include "../F6-06/bench.h"

__global__ void copyScalar(const float* in, float* out, int n)
{
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i < n) {
        out[i] = in[i];
    }
}

__global__ void copyVector(const float4* in, float4* out, int n4)   // n4 = n / 4
{
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i < n4) {
        out[i] = in[i];
    }
}

int main()
{
    const int n = 1 << 26;                                  // floats; a multiple of 4
    const std::size_t bytes = static_cast<std::size_t>(n) * sizeof(float);
    float* dIn = nullptr;
    float* dOut = nullptr;
    CUDA_CHECK(cudaMalloc(&dIn, bytes));
    CUDA_CHECK(cudaMalloc(&dOut, bytes));
    CUDA_CHECK(cudaMemset(dIn, 0, bytes));
    cudaEvent_t start = nullptr;
    cudaEvent_t stop = nullptr;
    CUDA_CHECK(cudaEventCreate(&start));
    CUDA_CHECK(cudaEventCreate(&stop));
    auto timed = [&](auto work) {
        return [&, work]() {
            CUDA_CHECK(cudaEventRecord(start));
            work();
            CUDA_CHECK(cudaEventRecord(stop));
            CUDA_CHECK_LAUNCH();
            CUDA_CHECK(cudaEventSynchronize(stop));
            float ms = 0.0f;
            CUDA_CHECK(cudaEventElapsedTime(&ms, start, stop));
            return static_cast<double>(ms);
        };
    };
    std::vector<bench::Row> rows;
    const int threads = 256;
    auto scalar = timed([&]() { copyScalar<<<(n + threads - 1) / threads, threads>>>(dIn, dOut, n); });
    rows.push_back({"cuda", "copy float", n, 2.0 * bytes, bench::summarize(bench::measure(scalar, 3, 21))});
    const int n4 = n / 4;
    auto vec = timed([&]() {
        copyVector<<<(n4 + threads - 1) / threads, threads>>>(reinterpret_cast<const float4*>(dIn),
                                                              reinterpret_cast<float4*>(dOut), n4);
    });
    rows.push_back({"cuda", "copy float4", n, 2.0 * bytes, bench::summarize(bench::measure(vec, 3, 21))});

    std::vector<float> pageable(n, 1.0f);                   // ordinary (pageable) host memory
    float* pinned = nullptr;
    CUDA_CHECK(cudaMallocHost(&pinned, bytes));             // page-locked host memory
    for (int i = 0; i < n; ++i) { pinned[i] = 1.0f; }
    auto h2dPageable = timed([&]() { CUDA_CHECK(cudaMemcpy(dIn, pageable.data(), bytes, cudaMemcpyHostToDevice)); });
    auto h2dPinned = timed([&]() { CUDA_CHECK(cudaMemcpy(dIn, pinned, bytes, cudaMemcpyHostToDevice)); });
    auto d2hPageable = timed([&]() { CUDA_CHECK(cudaMemcpy(pageable.data(), dIn, bytes, cudaMemcpyDeviceToHost)); });
    auto d2hPinned = timed([&]() { CUDA_CHECK(cudaMemcpy(pinned, dIn, bytes, cudaMemcpyDeviceToHost)); });
    rows.push_back({"cuda", "H2D pageable", n, 1.0 * bytes, bench::summarize(bench::measure(h2dPageable, 3, 21))});
    rows.push_back({"cuda", "H2D pinned", n, 1.0 * bytes, bench::summarize(bench::measure(h2dPinned, 3, 21))});
    rows.push_back({"cuda", "D2H pageable", n, 1.0 * bytes, bench::summarize(bench::measure(d2hPageable, 3, 21))});
    rows.push_back({"cuda", "D2H pinned", n, 1.0 * bytes, bench::summarize(bench::measure(d2hPinned, 3, 21))});
    bench::printTable(rows);
    std::printf("pinned / pageable H2D bandwidth ratio: %.2f\n",
                rows[2].stats.medianMs / rows[3].stats.medianMs);

    CUDA_CHECK(cudaFreeHost(pinned));
    CUDA_CHECK(cudaEventDestroy(start));
    CUDA_CHECK(cudaEventDestroy(stop));
    CUDA_CHECK(cudaFree(dIn));
    CUDA_CHECK(cudaFree(dOut));
    return 0;
}
