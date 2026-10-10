// F7-10 Listing 3 (forensic lab): a 32-tap filter. Each work-item computes OUT outputs,
// so each tap value loaded is reused OUT times. The team changed OUT from 16 to 32.
// Complete program; the build container has no AMD GPU, so the run stops at the first call.
#include <cstdio>
#include <cstdlib>
#include <hip/hip_runtime.h>

constexpr int TAPS = 32;

template <int OUT>
__global__ void fir(const float* x, const float* taps, float* y, int n)
{
    int base = (blockIdx.x * blockDim.x + threadIdx.x) * OUT;
    if (base + OUT + TAPS > n) {
        return;
    }
    float win[OUT + TAPS - 1];             // the input window this work-item needs
#pragma unroll
    for (int k = 0; k < OUT + TAPS - 1; ++k) {
        win[k] = x[base + k];
    }
    float acc[OUT];
#pragma unroll
    for (int o = 0; o < OUT; ++o) {
        acc[o] = 0.0f;
    }
#pragma unroll
    for (int t = 0; t < TAPS; ++t) {
        float w = taps[t];                 // same for every work-item: a scalar load
#pragma unroll
        for (int o = 0; o < OUT; ++o) {
            acc[o] += w * win[o + t];
        }
    }
#pragma unroll
    for (int o = 0; o < OUT; ++o) {
        y[base + o] = acc[o];
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
    const int n = 1 << 20;
    float *dX = nullptr, *dT = nullptr, *dY = nullptr;
    check(hipMalloc(&dX, n * sizeof(float)), "hipMalloc x");
    check(hipMalloc(&dT, TAPS * sizeof(float)), "hipMalloc taps");
    check(hipMalloc(&dY, n * sizeof(float)), "hipMalloc y");
    fir<16><<<n / 16 / 256, 256>>>(dX, dT, dY, n);    // before the change
    check(hipGetLastError(), "launch fir<16>");
    fir<32><<<n / 32 / 256, 256>>>(dX, dT, dY, n);    // after the change
    check(hipGetLastError(), "launch fir<32>");
    check(hipDeviceSynchronize(), "hipDeviceSynchronize");
    check(hipFree(dX), "hipFree x");
    check(hipFree(dT), "hipFree taps");
    check(hipFree(dY), "hipFree y");
    return 0;
}
