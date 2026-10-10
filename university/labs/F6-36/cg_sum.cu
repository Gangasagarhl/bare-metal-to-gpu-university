// F6-36 Listing 1: cooperative groups in three sizes.
//   tileSum   - a block reduction written with thread_block_tile<32> and cg::reduce
//   gridSum   - one launch that sums the whole array: every block writes a partial, the
//               whole grid waits at grid.sync(), then block 0 adds the partials
// The grid-wide kernel is launched with cudaLaunchCooperativeKernel and a grid no larger
// than the number of blocks that can be resident at once (queried, never guessed).
// Untested on hardware: the build container has no GPU.
#include <cooperative_groups.h>
#include <cooperative_groups/reduce.h>
#include <cstdio>
#include <cstdlib>
#include <vector>
#include <cuda_runtime.h>

namespace cg = cooperative_groups;

static void check(cudaError_t err, const char* what)
{
    if (err != cudaSuccess) {
        std::fprintf(stderr, "%s failed: %s (%s)\n", what, cudaGetErrorName(err), cudaGetErrorString(err));
        std::exit(EXIT_FAILURE);
    }
}

constexpr int kThreads = 256;

// sum of this thread's grid-stride elements, then of the block (result valid in thread 0)
__device__ float blockSum(const float* x, long long n)
{
    cg::thread_block block = cg::this_thread_block();
    cg::thread_block_tile<32> tile = cg::tiled_partition<32>(block);
    __shared__ float tileSums[kThreads / 32];
    float v = 0.0f;
    const long long stride = static_cast<long long>(gridDim.x) * blockDim.x;
    for (long long i = static_cast<long long>(blockIdx.x) * blockDim.x + threadIdx.x; i < n; i += stride) {
        v += x[i];
    }
    v = cg::reduce(tile, v, cg::plus<float>());           // 32 values -> 1, inside the tile
    if (tile.thread_rank() == 0) {
        tileSums[tile.meta_group_rank()] = v;             // one slot per tile of the block
    }
    block.sync();                                         // same barrier as __syncthreads()
    float total = 0.0f;
    if (block.thread_rank() == 0) {
        for (unsigned t = 0; t < tile.meta_group_size(); ++t) {
            total += tileSums[t];
        }
    }
    return total;
}

__global__ void tileSum(const float* x, float* partials, long long n)
{
    const float s = blockSum(x, n);
    if (threadIdx.x == 0) {
        partials[blockIdx.x] = s;
    }
}

__global__ void gridSum(const float* x, float* partials, float* result, long long n)
{
    cg::grid_group grid = cg::this_grid();
    const float s = blockSum(x, n);
    if (threadIdx.x == 0) {
        partials[blockIdx.x] = s;
    }
    grid.sync();                                          // every block's partial is now visible
    if (grid.block_rank() == 0 && threadIdx.x == 0) {
        float total = 0.0f;
        for (unsigned b = 0; b < gridDim.x; ++b) {
            total += partials[b];
        }
        *result = total;
    }
}

int main()
{
    const long long n = 10'000'000;
    std::vector<float> hX(static_cast<std::size_t>(n), 0.5f);
    int dev = 0;
    check(cudaGetDevice(&dev), "cudaGetDevice");
    int coop = 0;
    check(cudaDeviceGetAttribute(&coop, cudaDevAttrCooperativeLaunch, dev), "cooperative attribute");
    int sms = 0;
    check(cudaDeviceGetAttribute(&sms, cudaDevAttrMultiProcessorCount, dev), "SM count");
    int perSm = 0;
    check(cudaOccupancyMaxActiveBlocksPerMultiprocessor(&perSm, gridSum, kThreads, 0), "occupancy");
    const int blocks = sms * perSm;                       // the most blocks that can be resident
    std::printf("cooperative launch supported: %d; SMs %d x resident blocks per SM %d = grid %d\n",
                coop, sms, perSm, blocks);
    if (!coop || blocks == 0) {
        std::printf("this device cannot run gridSum\n");
        return EXIT_FAILURE;
    }
    float* dX = nullptr;
    float* dPartials = nullptr;
    float* dResult = nullptr;
    check(cudaMalloc(&dX, hX.size() * sizeof(float)), "cudaMalloc x");
    check(cudaMalloc(&dPartials, static_cast<std::size_t>(blocks) * sizeof(float)), "cudaMalloc partials");
    check(cudaMalloc(&dResult, sizeof(float)), "cudaMalloc result");
    check(cudaMemcpy(dX, hX.data(), hX.size() * sizeof(float), cudaMemcpyHostToDevice), "copy x");
    long long nArg = n;
    void* args[] = {&dX, &dPartials, &dResult, &nArg};
    check(cudaLaunchCooperativeKernel(reinterpret_cast<void*>(gridSum), dim3(blocks), dim3(kThreads), args, 0, nullptr),
          "cudaLaunchCooperativeKernel");
    check(cudaDeviceSynchronize(), "gridSum");
    float result = 0.0f;
    check(cudaMemcpy(&result, dResult, sizeof(float), cudaMemcpyDeviceToHost), "copy result");
    std::printf("gridSum: %.1f (expected %.1f)\n", static_cast<double>(result), 0.5 * static_cast<double>(n));
    tileSum<<<blocks, kThreads>>>(dX, dPartials, n);       // ordinary launch: no grid.sync inside
    check(cudaGetLastError(), "tileSum launch");
    std::vector<float> hP(static_cast<std::size_t>(blocks));
    check(cudaMemcpy(hP.data(), dPartials, hP.size() * sizeof(float), cudaMemcpyDeviceToHost), "copy partials");
    double total = 0.0;
    for (float p : hP) {
        total += p;
    }
    std::printf("tileSum + host: %.1f\n", total);
    check(cudaFree(dX), "cudaFree x");
    check(cudaFree(dPartials), "cudaFree partials");
    check(cudaFree(dResult), "cudaFree result");
    return 0;
}
