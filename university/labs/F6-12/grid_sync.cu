// F6-12 Listing 3: two correct ways to make every block finish phase 1 before phase 2 starts.
//   A: two kernel launches (the end of a kernel is a barrier for the whole grid)
//   B: one cooperative launch with cooperative_groups::this_grid().sync()
// Both sum n integers: phase 1 = per-block partial sums, phase 2 = block 0 adds the partials.
// Built for real; untested on hardware.
#include <cstdio>
#include <cstdlib>
#include <vector>
#include <cooperative_groups.h>
#include <cuda_runtime.h>

namespace cg = cooperative_groups;
constexpr int BLOCK = 256;

__device__ void blockPartial(const int* in, int n, long long* partial)
{
    __shared__ long long s[BLOCK];
    long long sum = 0;
    for (long long i = blockIdx.x * static_cast<long long>(blockDim.x) + threadIdx.x; i < n;
         i += static_cast<long long>(gridDim.x) * blockDim.x) {
        sum += in[i];
    }
    s[threadIdx.x] = sum;
    __syncthreads();
    for (int stride = blockDim.x / 2; stride > 0; stride /= 2) {
        if (threadIdx.x < stride) { s[threadIdx.x] += s[threadIdx.x + stride]; }
        __syncthreads();
    }
    if (threadIdx.x == 0) { partial[blockIdx.x] = s[0]; }
}

__global__ void phase1(const int* in, int n, long long* partial)
{
    blockPartial(in, n, partial);
}

__global__ void phase2(const long long* partial, int parts, long long* total)
{
    if (blockIdx.x == 0 && threadIdx.x == 0) {
        long long t = 0;
        for (int b = 0; b < parts; ++b) { t += partial[b]; }
        *total = t;
    }
}

__global__ void bothPhases(const int* in, int n, long long* partial, long long* total)
{
    blockPartial(in, n, partial);
    cg::this_grid().sync();                     // every block has written its partial
    if (blockIdx.x == 0 && threadIdx.x == 0) {
        long long t = 0;
        for (unsigned b = 0; b < gridDim.x; ++b) { t += partial[b]; }
        *total = t;
    }
}

static void check(cudaError_t err, const char* what)
{
    if (err != cudaSuccess) {
        std::fprintf(stderr, "%s failed: %s (%s)\n", what, cudaGetErrorName(err), cudaGetErrorString(err));
        std::exit(EXIT_FAILURE);
    }
}

int main()
{
    int dev = 0;
    check(cudaGetDevice(&dev), "cudaGetDevice");
    int coop = 0, sms = 0, perSm = 0;
    check(cudaDeviceGetAttribute(&coop, cudaDevAttrCooperativeLaunch, dev), "attribute CooperativeLaunch");
    check(cudaDeviceGetAttribute(&sms, cudaDevAttrMultiProcessorCount, dev), "attribute MultiProcessorCount");
    check(cudaOccupancyMaxActiveBlocksPerMultiprocessor(&perSm, bothPhases, BLOCK, 0), "occupancy");
    const int grid = sms * perSm;               // the most blocks that can all be resident at once
    std::printf("cooperative launch supported: %d; SMs %d x resident blocks %d = grid %d\n", coop, sms, perSm, grid);

    const int n = 10000019;
    std::vector<int> h(n);
    long long expect = 0;
    for (int i = 0; i < n; ++i) { h[i] = i % 1000; expect += h[i]; }
    int* dIn = nullptr;
    long long* dPartial = nullptr;
    long long* dTotal = nullptr;
    check(cudaMalloc(&dIn, sizeof(int) * n), "cudaMalloc in");
    check(cudaMalloc(&dPartial, sizeof(long long) * grid), "cudaMalloc partial");
    check(cudaMalloc(&dTotal, sizeof(long long)), "cudaMalloc total");
    check(cudaMemcpy(dIn, h.data(), sizeof(int) * n, cudaMemcpyHostToDevice), "copy in");

    long long got = 0;
    phase1<<<grid, BLOCK>>>(dIn, n, dPartial);                         // A: two launches
    check(cudaGetLastError(), "launch phase1");
    phase2<<<1, 1>>>(dPartial, grid, dTotal);
    check(cudaGetLastError(), "launch phase2");
    check(cudaMemcpy(&got, dTotal, sizeof got, cudaMemcpyDeviceToHost), "copy total A");
    std::printf("A two launches:      %lld (expected %lld)\n", got, expect);

    if (coop) {                                                       // B: one cooperative launch
        int nArg = n;
        void* args[] = {&dIn, &nArg, &dPartial, &dTotal};
        check(cudaLaunchCooperativeKernel(reinterpret_cast<void*>(bothPhases), grid, BLOCK, args, 0, 0),
              "cudaLaunchCooperativeKernel");
        check(cudaMemcpy(&got, dTotal, sizeof got, cudaMemcpyDeviceToHost), "copy total B");
        std::printf("B cooperative launch: %lld (expected %lld)\n", got, expect);
    }
    check(cudaFree(dIn), "cudaFree in");
    check(cudaFree(dPartial), "cudaFree partial");
    check(cudaFree(dTotal), "cudaFree total");
    return 0;
}
