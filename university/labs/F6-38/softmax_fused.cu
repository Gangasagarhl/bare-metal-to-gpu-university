// F6-38 Listing 4: fused row softmax and layer norm, one warp per row.
// Each kernel reads its row once for the statistics and once for the output, and keeps
// everything in between in registers: no temporary arrays in global memory.
// Untested on hardware: the build container has no GPU.
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <vector>
#include <cuda_runtime.h>

static void check(cudaError_t err, const char* what)
{
    if (err != cudaSuccess) {
        std::fprintf(stderr, "%s failed: %s (%s)\n", what, cudaGetErrorName(err), cudaGetErrorString(err));
        std::exit(EXIT_FAILURE);
    }
}

constexpr int kWarp = 32;
constexpr unsigned kFull = 0xffffffffu;

// merge two online-softmax states (m, d): the larger maximum wins, the other sum is rescaled
__device__ void mergeMaxSum(float& m, float& d, float mOther, float dOther)
{
    const float mNew = fmaxf(m, mOther);
    if (mNew == -INFINITY) {
        return;                                   // both states empty: nothing to rescale
    }
    d = d * __expf(m - mNew) + dOther * __expf(mOther - mNew);
    m = mNew;
}

__global__ void softmaxRows(const float* x, float* y, int rows, int cols)
{
    const int row = blockIdx.x * (blockDim.x / kWarp) + threadIdx.x / kWarp;
    const int lane = threadIdx.x % kWarp;
    if (row >= rows) {
        return;                                   // whole warp leaves together
    }
    const float* xr = x + static_cast<long long>(row) * cols;
    float m = -INFINITY;
    float d = 0.0f;
    for (int c = lane; c < cols; c += kWarp) {     // pass 1: each lane keeps its own (m, d)
        const float v = xr[c];
        const float mNew = fmaxf(m, v);
        d = d * __expf(m - mNew) + __expf(v - mNew);
        m = mNew;
    }
    for (int off = kWarp / 2; off > 0; off /= 2) { // merge the 32 states with shuffles
        const float mO = __shfl_xor_sync(kFull, m, off);
        const float dO = __shfl_xor_sync(kFull, d, off);
        mergeMaxSum(m, d, mO, dO);
    }
    float* yr = y + static_cast<long long>(row) * cols;
    for (int c = lane; c < cols; c += kWarp) {     // pass 2: write the normalised values
        yr[c] = __expf(xr[c] - m) / d;
    }
}

__global__ void layerNormRows(const float* x, float* y, int rows, int cols, float eps)
{
    const int row = blockIdx.x * (blockDim.x / kWarp) + threadIdx.x / kWarp;
    const int lane = threadIdx.x % kWarp;
    if (row >= rows) {
        return;
    }
    const float* xr = x + static_cast<long long>(row) * cols;
    float n = 0.0f;
    float mean = 0.0f;
    float m2 = 0.0f;
    for (int c = lane; c < cols; c += kWarp) {     // Welford, one element at a time
        const float v = xr[c];
        n += 1.0f;
        const float delta = v - mean;
        mean += delta / n;
        m2 += delta * (v - mean);
    }
    for (int off = kWarp / 2; off > 0; off /= 2) { // merge Welford states across the warp
        const float nO = __shfl_xor_sync(kFull, n, off);
        const float meanO = __shfl_xor_sync(kFull, mean, off);
        const float m2O = __shfl_xor_sync(kFull, m2, off);
        const float nT = n + nO;
        if (nT > 0.0f) {
            const float delta = meanO - mean;
            mean += delta * (nO / nT);
            m2 += m2O + delta * delta * (n * nO / nT);
        }
        n = nT;
    }
    const float rstd = rsqrtf(m2 / n + eps);
    float* yr = y + static_cast<long long>(row) * cols;
    for (int c = lane; c < cols; c += kWarp) {
        yr[c] = (xr[c] - mean) * rstd;
    }
}

int main()
{
    const int rows = 1000;
    const int cols = 1500;                         // not a multiple of 32 on purpose
    const std::size_t count = static_cast<std::size_t>(rows) * cols;
    std::vector<float> hX(count);
    for (std::size_t i = 0; i < count; ++i) {
        hX[i] = 100.0f + static_cast<float>(i % 97) * 0.25f;   // large values: naive exp overflows
    }
    float* dX = nullptr;
    float* dY = nullptr;
    check(cudaMalloc(&dX, count * sizeof(float)), "cudaMalloc x");
    check(cudaMalloc(&dY, count * sizeof(float)), "cudaMalloc y");
    check(cudaMemcpy(dX, hX.data(), count * sizeof(float), cudaMemcpyHostToDevice), "copy x");
    const int threads = 256;                       // 8 warps = 8 rows per block
    const int blocks = (rows + threads / kWarp - 1) / (threads / kWarp);
    softmaxRows<<<blocks, threads>>>(dX, dY, rows, cols);
    check(cudaGetLastError(), "softmaxRows launch");
    std::vector<float> hY(count);
    check(cudaMemcpy(hY.data(), dY, count * sizeof(float), cudaMemcpyDeviceToHost), "copy y");
    double worst = 0.0;
    for (int r = 0; r < rows; ++r) {
        double sum = 0.0;
        for (int c = 0; c < cols; ++c) {
            sum += hY[static_cast<std::size_t>(r) * cols + c];
        }
        worst = std::fmax(worst, std::fabs(sum - 1.0));
    }
    std::printf("softmax: worst |row sum - 1| = %g\n", worst);
    layerNormRows<<<blocks, threads>>>(dX, dY, rows, cols, 1e-5f);
    check(cudaGetLastError(), "layerNormRows launch");
    check(cudaMemcpy(hY.data(), dY, count * sizeof(float), cudaMemcpyDeviceToHost), "copy y");
    double worstMean = 0.0;
    for (int r = 0; r < rows; ++r) {
        double sum = 0.0;
        for (int c = 0; c < cols; ++c) {
            sum += hY[static_cast<std::size_t>(r) * cols + c];
        }
        worstMean = std::fmax(worstMean, std::fabs(sum / cols));
    }
    std::printf("layer norm: worst |row mean| = %g\n", worstMean);
    check(cudaFree(dX), "cudaFree x");
    check(cudaFree(dY), "cudaFree y");
    return 0;
}
