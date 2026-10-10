// F6-06 Listing 4: the HIP slot of the harness. The same measurement with HIP events,
// so that the HIP courses (F7) can plug into bench.h unchanged. Compiled for gfx90a.
#include <cstdio>
#include <cstdlib>
#include <vector>
#include <hip/hip_runtime.h>
#include "bench.h"

static void hipCheck(hipError_t err, const char* call, int line)
{
    if (err != hipSuccess) {
        std::fprintf(stderr, "timing_hip.hip:%d: %s failed: %s\n", line, call, hipGetErrorName(err));
        std::exit(EXIT_FAILURE);
    }
}
#define HIP_CHECK(call) hipCheck((call), #call, __LINE__)

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
    HIP_CHECK(hipMalloc(&dX, bytes));
    HIP_CHECK(hipMalloc(&dY, bytes));
    HIP_CHECK(hipMemcpy(dX, hX.data(), bytes, hipMemcpyHostToDevice));
    HIP_CHECK(hipMemcpy(dY, hY.data(), bytes, hipMemcpyHostToDevice));
    hipEvent_t start = nullptr;
    hipEvent_t stop = nullptr;
    HIP_CHECK(hipEventCreate(&start));
    HIP_CHECK(hipEventCreate(&stop));
    const int threads = 256;
    const int blocks = (n + threads - 1) / threads;
    auto timeOnce = [&]() {
        HIP_CHECK(hipEventRecord(start, nullptr));
        saxpy<<<blocks, threads>>>(n, 0.5f, dX, dY);
        HIP_CHECK(hipEventRecord(stop, nullptr));
        HIP_CHECK(hipGetLastError());
        HIP_CHECK(hipEventSynchronize(stop));
        float ms = 0.0f;
        HIP_CHECK(hipEventElapsedTime(&ms, start, stop));
        return static_cast<double>(ms);
    };
    const std::vector<double> runs = bench::measure(timeOnce, 3, 21);
    bench::Row row{"hip", "saxpy", n, 3.0 * bytes, bench::summarize(runs)};
    bench::printTable({row});
    HIP_CHECK(hipEventDestroy(start));
    HIP_CHECK(hipEventDestroy(stop));
    HIP_CHECK(hipFree(dX));
    HIP_CHECK(hipFree(dY));
    return 0;
}
