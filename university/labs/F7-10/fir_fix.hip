// F7-10 Listing 4 (forensic lab, fix candidates): the 32-output filter of Listing 3 with
// __launch_bounds__(256, MINWAVES). In HIP 5.7 the second argument becomes
// amdgpu_waves_per_eu(MINWAVES): a request for at least MINWAVES waves per SIMD, which caps
// the registers the compiler may use. Compare the compiler's report with fir<16> and fir<32>.
// Complete program; the build container has no AMD GPU, so the run stops at the first call.
#include <cstdio>
#include <cstdlib>
#include <hip/hip_runtime.h>

constexpr int TAPS = 32;

template <int OUT, int MINWAVES>
__global__ void __launch_bounds__(256, MINWAVES) firLB(const float* x, const float* taps, float* y, int n)
{
    int base = (blockIdx.x * blockDim.x + threadIdx.x) * OUT;
    if (base + OUT + TAPS > n) {
        return;
    }
    float win[OUT + TAPS - 1];
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
        float w = taps[t];
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
    firLB<32, 6><<<n / 32 / 256, 256>>>(dX, dT, dY, n);   // ask for at least 6 waves per SIMD
    check(hipGetLastError(), "launch firLB<32, 6>");
    firLB<32, 8><<<n / 32 / 256, 256>>>(dX, dT, dY, n);   // ask for 8 waves per SIMD
    check(hipGetLastError(), "launch firLB<32, 8>");
    check(hipDeviceSynchronize(), "hipDeviceSynchronize");
    check(hipFree(dX), "hipFree x");
    check(hipFree(dT), "hipFree taps");
    check(hipFree(dY), "hipFree y");
    return 0;
}
