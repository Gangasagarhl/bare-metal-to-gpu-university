// F7-14 Listing 1: timing a kernel with HIP events, following the measurement protocol of
// curriculum 13.4 (warm-up, then the median of 20 timed runs). A profiler is the next step.
// Complete program; the build container has no AMD GPU, so the run stops at the first call.
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <vector>
#include <hip/hip_runtime.h>

__global__ void scale(const float* x, float* y, float a, int n)
{
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i < n) {
        y[i] = a * x[i];
    }
}

static void check(hipError_t err, const char* what)
{
    if (err != hipSuccess) {
        std::fprintf(stderr, "%s failed: %s (%s)\n", what, hipGetErrorName(err), hipGetErrorString(err));
        std::exit(EXIT_FAILURE);
    }
}

int main()
{
    const int n = 1 << 24;
    float *dX = nullptr, *dY = nullptr;
    check(hipMalloc(&dX, n * sizeof(float)), "hipMalloc x");
    check(hipMalloc(&dY, n * sizeof(float)), "hipMalloc y");
    check(hipMemset(dX, 0, n * sizeof(float)), "hipMemset x");
    hipEvent_t start, stop;
    check(hipEventCreate(&start), "hipEventCreate start");
    check(hipEventCreate(&stop), "hipEventCreate stop");
    const int blocks = (n + 255) / 256;
    for (int w = 0; w < 3; ++w) {                                   // warm-up runs
        scale<<<blocks, 256>>>(dX, dY, 2.0f, n);
    }
    check(hipGetLastError(), "warm-up launches");
    std::vector<float> ms(20);
    for (float& t : ms) {
        check(hipEventRecord(start, nullptr), "hipEventRecord start");
        scale<<<blocks, 256>>>(dX, dY, 2.0f, n);
        check(hipEventRecord(stop, nullptr), "hipEventRecord stop");
        check(hipEventSynchronize(stop), "hipEventSynchronize");
        check(hipEventElapsedTime(&t, start, stop), "hipEventElapsedTime");
    }
    std::sort(ms.begin(), ms.end());
    const double median = 0.5 * (ms[9] + ms[10]);
    const double gbps = 2.0 * n * sizeof(float) / (median * 1e-3) / 1e9;   // read x, write y
    std::printf("median of 20: %.4f ms, %.1f GB/s (decimal GB)\n", median, gbps);
    check(hipEventDestroy(start), "hipEventDestroy start");
    check(hipEventDestroy(stop), "hipEventDestroy stop");
    check(hipFree(dX), "hipFree x");
    check(hipFree(dY), "hipFree y");
    return 0;
}
