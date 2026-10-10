// F6-35 Listing 4: CUB at two levels.
//   device level: cub::DeviceReduce::Sum, called twice (first to ask how much temporary storage
//                 it needs, then to do the work)
//   block level : cub::BlockReduce inside our own kernel, the building block for fused kernels
// Untested on hardware: the build container has no GPU.
#include <cub/cub.cuh>
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

constexpr int kThreads = 256;

// each block squares its 256 elements and sums them: a fused "square + reduce"
__global__ void sumSquaresPerBlock(const float* x, float* blockSums, int n)
{
    using BlockReduce = cub::BlockReduce<float, kThreads>;
    __shared__ typename BlockReduce::TempStorage temp;
    const int i = blockIdx.x * kThreads + threadIdx.x;
    const float v = (i < n) ? x[i] * x[i] : 0.0f;
    const float s = BlockReduce(temp).Sum(v);             // valid in thread 0 only
    if (threadIdx.x == 0) {
        blockSums[blockIdx.x] = s;
    }
}

int main()
{
    const int n = 1'000'003;                               // not a multiple of 256
    std::vector<float> hX(static_cast<std::size_t>(n), 0.5f);
    const int blocks = (n + kThreads - 1) / kThreads;
    float* dX = nullptr;
    float* dBlockSums = nullptr;
    float* dTotal = nullptr;
    check(cudaMalloc(&dX, hX.size() * sizeof(float)), "cudaMalloc x");
    check(cudaMalloc(&dBlockSums, static_cast<std::size_t>(blocks) * sizeof(float)), "cudaMalloc blockSums");
    check(cudaMalloc(&dTotal, sizeof(float)), "cudaMalloc total");
    check(cudaMemcpy(dX, hX.data(), hX.size() * sizeof(float), cudaMemcpyHostToDevice), "copy x");
    sumSquaresPerBlock<<<blocks, kThreads>>>(dX, dBlockSums, n);
    check(cudaGetLastError(), "sumSquaresPerBlock launch");
    std::size_t tempBytes = 0;
    check(cub::DeviceReduce::Sum(nullptr, tempBytes, dBlockSums, dTotal, blocks), "DeviceReduce size query");
    void* dTemp = nullptr;
    check(cudaMalloc(&dTemp, tempBytes), "cudaMalloc temp");
    check(cub::DeviceReduce::Sum(dTemp, tempBytes, dBlockSums, dTotal, blocks), "DeviceReduce::Sum");
    float total = 0.0f;
    check(cudaMemcpy(&total, dTotal, sizeof(float), cudaMemcpyDeviceToHost), "copy total");
    std::printf("sum of squares %.2f (expected %.2f), temporary storage %zu bytes\n",
                static_cast<double>(total), 0.25 * n, tempBytes);
    check(cudaFree(dTemp), "cudaFree temp");
    check(cudaFree(dX), "cudaFree x");
    check(cudaFree(dBlockSums), "cudaFree blockSums");
    check(cudaFree(dTotal), "cudaFree total");
    return 0;
}
