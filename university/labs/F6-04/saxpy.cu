// F6-04 Listing 2: SAXPY (y = a*x + y) on the GPU, checked bit for bit against a CPU
// reference for sizes from 1 to 100 million (curriculum milestone E1).
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>
#include <cuda_runtime.h>

__global__ void saxpy(int n, float a, const float* x, float* y)
{
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i < n) {
        y[i] = a * x[i] + y[i];
    }
}

static void check(cudaError_t err, const char* what)
{
    if (err != cudaSuccess) {
        std::fprintf(stderr, "%s failed: %s (%s)\n", what, cudaGetErrorName(err),
                     cudaGetErrorString(err));
        std::exit(EXIT_FAILURE);
    }
}

// The reference rounds once, like a fused multiply-add (see the chapter's Worked example).
static void saxpyReference(float a, const std::vector<float>& x, std::vector<float>& y)
{
    for (std::size_t i = 0; i < x.size(); ++i) {
        y[i] = std::fma(a, x[i], y[i]);
    }
}

static std::uint32_t bits(float f)
{
    std::uint32_t u = 0;
    std::memcpy(&u, &f, sizeof u);
    return u;
}

// Deterministic test data with many different mantissas (no library random generator).
static float value(std::uint32_t i, std::uint32_t seed)
{
    std::uint32_t h = (i + seed) * 2654435761u;
    h ^= h >> 15;
    return static_cast<float>(h % 2000003u) / 1000.0f - 1000.0f;
}

int main()
{
    int devices = 0;
    check(cudaGetDeviceCount(&devices), "cudaGetDeviceCount");
    const float a = 1.7f;
    const int sizes[] = {1, 7, 255, 256, 257, 1000, 1 << 20, 1000003, 100000000};
    int failures = 0;
    for (int n : sizes) {
        std::vector<float> x(n), y(n), ref(n);
        for (int i = 0; i < n; ++i) { x[i] = value(i, 1); y[i] = value(i, 2); }
        ref = y;
        saxpyReference(a, x, ref);

        const std::size_t bytes = static_cast<std::size_t>(n) * sizeof(float);
        float* dX = nullptr;
        float* dY = nullptr;
        check(cudaMalloc(&dX, bytes), "cudaMalloc x");
        check(cudaMalloc(&dY, bytes), "cudaMalloc y");
        check(cudaMemcpy(dX, x.data(), bytes, cudaMemcpyHostToDevice), "copy x");
        check(cudaMemcpy(dY, y.data(), bytes, cudaMemcpyHostToDevice), "copy y");
        const int threads = 256;
        const int blocks = (n + threads - 1) / threads;
        saxpy<<<blocks, threads>>>(n, a, dX, dY);
        check(cudaGetLastError(), "saxpy launch");
        check(cudaMemcpy(y.data(), dY, bytes, cudaMemcpyDeviceToHost), "copy y back");
        check(cudaFree(dX), "cudaFree x");
        check(cudaFree(dY), "cudaFree y");

        int mismatches = 0;
        for (int i = 0; i < n; ++i) {
            if (bits(y[i]) != bits(ref[i])) { ++mismatches; }
        }
        std::printf("n = %9d  blocks = %6d  bit mismatches = %d\n", n, blocks, mismatches);
        if (mismatches != 0) { ++failures; }
    }
    std::printf("%s\n", failures == 0 ? "PASS: bit-exact for every size" : "FAIL");
    return failures == 0 ? 0 : 1;
}
