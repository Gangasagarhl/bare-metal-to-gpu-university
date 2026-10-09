// F0-80 Listing 3: the reduction kernel whose test this chapter designs. Built for real with nvcc;
// UNTESTED ON HARDWARE (the build container has no GPU). Same addition order as reduce_model.hpp.
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <vector>

#include <cuda_runtime.h>

static void check(cudaError_t err, const char* what)
{
    if (err != cudaSuccess) {
        std::fprintf(stderr, "CUDA error in %s: %s\n", what, cudaGetErrorString(err));
        std::exit(1);
    }
}

constexpr unsigned kThreads = 256;
constexpr unsigned kBlocks = 128;

__global__ void blockSums(const float* x, size_t n, float* partial)
{
    __shared__ float s[kThreads];
    const size_t stride = static_cast<size_t>(gridDim.x) * blockDim.x;
    float acc = 0.0f;
    for (size_t i = static_cast<size_t>(blockIdx.x) * blockDim.x + threadIdx.x; i < n;
         i += stride) {
        acc += x[i];
    }
    s[threadIdx.x] = acc;
    __syncthreads();
    for (unsigned half = blockDim.x / 2; half > 0; half /= 2) {
        if (threadIdx.x < half) {
            s[threadIdx.x] += s[threadIdx.x + half];
        }
        __syncthreads();
    }
    if (threadIdx.x == 0) {
        partial[blockIdx.x] = s[0];
    }
}

static uint64_t splitmix64(uint64_t& state)
{
    uint64_t z = (state += 0x9E3779B97F4A7C15ull);
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ull;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBull;
    return z ^ (z >> 31);
}

int main()
{
    const size_t n = (size_t{1} << 20) + 3;
    std::vector<float> hX(n);
    uint64_t seed = n + 1;
    for (auto& v : hX) {
        v = std::ldexp(static_cast<float>(splitmix64(seed) >> 40), -24);
    }
    float* dX = nullptr;
    float* dPartial = nullptr;
    check(cudaMalloc(&dX, n * sizeof(float)), "cudaMalloc x");
    check(cudaMalloc(&dPartial, kBlocks * sizeof(float)), "cudaMalloc partial");
    check(cudaMemcpy(dX, hX.data(), n * sizeof(float), cudaMemcpyHostToDevice),
          "cudaMemcpy to device");
    blockSums<<<kBlocks, kThreads>>>(dX, n, dPartial);
    check(cudaGetLastError(), "kernel launch");
    check(cudaDeviceSynchronize(), "kernel run");
    std::vector<float> hPartial(kBlocks);
    check(cudaMemcpy(hPartial.data(), dPartial, kBlocks * sizeof(float), cudaMemcpyDeviceToHost),
          "cudaMemcpy to host");
    float gpu = 0.0f;
    for (float p : hPartial) {
        gpu += p; // block partials added in order, as in the model
    }
    double ref = 0.0, c = 0.0, absSum = 0.0; // Kahan reference in double (E3)
    for (float v : hX) {
        const double y = static_cast<double>(v) - c;
        const double t = ref + y;
        c = (t - ref) - y;
        ref = t;
        absSum += std::fabs(static_cast<double>(v));
    }
    const double depth =
        std::ceil(static_cast<double>(n) / (kBlocks * kThreads)) - 1 + 8 + (kBlocks - 1);
    const double u = std::ldexp(1.0, -24);
    const double tol = depth * u / (1.0 - depth * u) * absSum + 4.0 * std::ldexp(1.0, -53) * absSum;
    const double err = std::fabs(static_cast<double>(gpu) - ref);
    std::printf("gpu = %.9g  reference = %.12g  |err| = %.3g  tol = %.3g  %s\n",
                static_cast<double>(gpu), ref, err, tol, err <= tol ? "PASS" : "FAIL");
    check(cudaFree(dX), "cudaFree x");
    check(cudaFree(dPartial), "cudaFree partial");
    return err <= tol ? 0 : 1;
}
