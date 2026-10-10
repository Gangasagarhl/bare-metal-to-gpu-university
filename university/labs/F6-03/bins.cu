// F6-03 Listing 2 (forensic evidence): two versions of a per-thread histogram.
// binsSlow indexes its private array with a value known only at run time;
// binsFixed touches every bin with a compile-time index instead.
#include <cstdio>
#include <cstdlib>
#include <vector>
#include <cuda_runtime.h>

constexpr int kBins = 32;
constexpr int kSlots = 1024;            // out has kBins rows of kSlots partial sums

__global__ void binsSlow(const int* keys, const float* vals, float* out, int n)
{
    float bins[kBins];
    for (int k = 0; k < kBins; ++k) { bins[k] = 0.0f; }
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    for (int j = i; j < n; j += blockDim.x * gridDim.x) {
        bins[keys[j] & (kBins - 1)] += vals[j];            // run-time index
    }
    for (int k = 0; k < kBins; ++k) { out[k * kSlots + i] = bins[k]; }
}

__global__ void binsFixed(const int* keys, const float* vals, float* out, int n)
{
    float bins[kBins];
#pragma unroll
    for (int k = 0; k < kBins; ++k) { bins[k] = 0.0f; }
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    for (int j = i; j < n; j += blockDim.x * gridDim.x) {
        const int key = keys[j] & (kBins - 1);
        const float v = vals[j];
#pragma unroll
        for (int k = 0; k < kBins; ++k) { bins[k] += (key == k) ? v : 0.0f; }  // fixed index
    }
#pragma unroll
    for (int k = 0; k < kBins; ++k) { out[k * kSlots + i] = bins[k]; }
}

static void check(cudaError_t err, const char* what)
{
    if (err != cudaSuccess) {
        std::fprintf(stderr, "%s failed: %s (%s)\n", what, cudaGetErrorName(err),
                     cudaGetErrorString(err));
        std::exit(EXIT_FAILURE);
    }
}

int main()
{
    const int n = 1 << 20;
    std::vector<int> hKeys(n);
    std::vector<float> hVals(n, 1.0f);
    for (int j = 0; j < n; ++j) { hKeys[j] = (j * 7) % 1000; }
    int* dKeys = nullptr;
    float* dVals = nullptr;
    float* dOut = nullptr;
    const std::size_t outBytes = kBins * kSlots * sizeof(float);
    check(cudaMalloc(&dKeys, n * sizeof(int)), "cudaMalloc keys");
    check(cudaMalloc(&dVals, n * sizeof(float)), "cudaMalloc vals");
    check(cudaMalloc(&dOut, outBytes), "cudaMalloc out");
    check(cudaMemcpy(dKeys, hKeys.data(), n * sizeof(int), cudaMemcpyHostToDevice), "copy keys");
    check(cudaMemcpy(dVals, hVals.data(), n * sizeof(float), cudaMemcpyHostToDevice), "copy vals");

    std::vector<float> slow(kBins * kSlots), fixed(kBins * kSlots);
    binsSlow<<<kSlots / 256, 256>>>(dKeys, dVals, dOut, n);    // exactly kSlots threads
    check(cudaGetLastError(), "binsSlow launch");
    check(cudaMemcpy(slow.data(), dOut, outBytes, cudaMemcpyDeviceToHost), "copy slow");
    binsFixed<<<kSlots / 256, 256>>>(dKeys, dVals, dOut, n);
    check(cudaGetLastError(), "binsFixed launch");
    check(cudaMemcpy(fixed.data(), dOut, outBytes, cudaMemcpyDeviceToHost), "copy fixed");

    int differ = 0;
    for (int k = 0; k < kBins * kSlots; ++k) {
        if (slow[k] != fixed[k]) { ++differ; }
    }
    std::printf("partial sums compared: %d, different: %d\n", kBins * kSlots, differ);
    check(cudaFree(dKeys), "cudaFree keys");
    check(cudaFree(dVals), "cudaFree vals");
    check(cudaFree(dOut), "cudaFree out");
    return differ == 0 ? 0 : 1;
}
