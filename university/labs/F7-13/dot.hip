// F7-13 Listing 1: a dot product with a grid-stride loop, 16-byte loads and a wave reduction.
// dot() writes the arithmetic as one expression; dotFma() as a chain of fmaf() calls.
// Complete program; the build container has no AMD GPU, so the run stops at the first call.
#include <cstdio>
#include <cstdlib>
#include <vector>
#include <hip/hip_runtime.h>

__device__ float waveSum(float v)
{
    for (int d = warpSize / 2; d > 0; d /= 2) {
        v += __shfl_down(v, d);
    }
    return v;
}

__global__ void dot(const float4* a, const float4* b, float* result, int n4)
{
    float acc = 0.0f;
    for (int i = blockIdx.x * blockDim.x + threadIdx.x; i < n4; i += gridDim.x * blockDim.x) {
        float4 x = a[i];
        float4 y = b[i];
        acc += x.x * y.x + x.y * y.y + x.z * y.z + x.w * y.w;
    }
    acc = waveSum(acc);
    if (__lane_id() == 0) {
        atomicAdd(result, acc);
    }
}

__global__ void dotFma(const float4* a, const float4* b, float* result, int n4)
{
    float acc = 0.0f;
    for (int i = blockIdx.x * blockDim.x + threadIdx.x; i < n4; i += gridDim.x * blockDim.x) {
        float4 x = a[i];
        float4 y = b[i];
        acc = fmaf(x.x, y.x, acc);
        acc = fmaf(x.y, y.y, acc);
        acc = fmaf(x.z, y.z, acc);
        acc = fmaf(x.w, y.w, acc);
    }
    acc = waveSum(acc);
    if (__lane_id() == 0) {
        atomicAdd(result, acc);
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
    const int n4 = 1 << 18;                      // 2^20 floats
    std::vector<float> hA(4 * n4, 1.0f), hB(4 * n4, 0.5f);
    float4 *dA = nullptr, *dB = nullptr;
    float* dR = nullptr;
    check(hipMalloc(&dA, hA.size() * sizeof(float)), "hipMalloc a");
    check(hipMalloc(&dB, hB.size() * sizeof(float)), "hipMalloc b");
    check(hipMalloc(&dR, 2 * sizeof(float)), "hipMalloc result");
    check(hipMemcpy(dA, hA.data(), hA.size() * sizeof(float), hipMemcpyHostToDevice), "copy a");
    check(hipMemcpy(dB, hB.data(), hB.size() * sizeof(float), hipMemcpyHostToDevice), "copy b");
    check(hipMemset(dR, 0, 2 * sizeof(float)), "hipMemset result");
    dot<<<256, 256>>>(dA, dB, dR, n4);
    check(hipGetLastError(), "launch dot");
    dotFma<<<256, 256>>>(dA, dB, dR + 1, n4);
    check(hipGetLastError(), "launch dotFma");
    float hR[2] = {0.0f, 0.0f};
    check(hipMemcpy(hR, dR, sizeof(hR), hipMemcpyDeviceToHost), "copy result");
    std::printf("dot %.1f, dotFma %.1f, expected %.1f\n", hR[0], hR[1], 0.5 * 4 * n4);
    check(hipFree(dA), "hipFree a");
    check(hipFree(dB), "hipFree b");
    check(hipFree(dR), "hipFree result");
    return 0;
}
