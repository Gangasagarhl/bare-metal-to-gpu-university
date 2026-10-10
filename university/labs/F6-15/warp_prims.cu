// F6-15 Listing 2: warp-level primitives in CUDA: reduce (down), all-reduce (xor),
// inclusive scan (up), compaction (ballot + popc), and the sm_80 reduce instruction.
// Built for real; untested on hardware.
#include <cstdio>
#include <cstdlib>
#include <vector>
#include <cuda_runtime.h>

constexpr unsigned FULL = 0xffffffffu;      // all 32 lanes take part (CUDA warps are 32 wide)

__device__ int warpReduce(int v)
{
    for (int off = warpSize / 2; off > 0; off /= 2) { v += __shfl_down_sync(FULL, v, off); }
    return v;                               // valid in lane 0
}

__device__ int warpAllReduce(int v)
{
    for (int m = warpSize / 2; m > 0; m /= 2) { v += __shfl_xor_sync(FULL, v, m); }
    return v;                               // valid in every lane
}

__device__ int warpInclusiveScan(int v)
{
    int lane = threadIdx.x % warpSize;
    for (int d = 1; d < warpSize; d *= 2) {
        int up = __shfl_up_sync(FULL, v, d);
        if (lane >= d) { v += up; }
    }
    return v;
}

__global__ void warpTests(const int* in, int* sums, int* allSums, int* scans, int* kept, int* keptCount)
{
    int i = blockIdx.x * blockDim.x + threadIdx.x;      // the grid is an exact number of warps
    int lane = threadIdx.x % warpSize;
    int warp = i / warpSize;
    int v = in[i];
    int s = warpReduce(v);
    if (lane == 0) { sums[warp] = s; }
    allSums[i] = warpAllReduce(v);
    scans[i] = warpInclusiveScan(v);
    bool keep = v % 3 == 0;
    unsigned mask = __ballot_sync(FULL, keep);
    if (keep) {
        int slot = __popc(mask & ((1u << lane) - 1));   // kept lanes below me
        kept[warp * warpSize + slot] = v;
    }
    if (lane == 0) { keptCount[warp] = __popc(mask); }
}

__global__ void hardwareReduce(const int* in, int* sums)
{
#if __CUDA_ARCH__ >= 800
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    int s = __reduce_add_sync(FULL, in[i]);             // one instruction on sm_80 and later
    if (threadIdx.x % warpSize == 0) { sums[i / warpSize] = s; }
#else
    (void)in; (void)sums;
#endif
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
    const int n = 1 << 20, threads = 256, warps = n / 32;
    std::vector<int> h(n);
    for (int i = 0; i < n; ++i) { h[i] = (i * 37) % 101; }
    int *dIn, *dSums, *dAll, *dScan, *dKept, *dCount;
    check(cudaMalloc(&dIn, sizeof(int) * n), "malloc");
    check(cudaMalloc(&dSums, sizeof(int) * warps), "malloc");
    check(cudaMalloc(&dAll, sizeof(int) * n), "malloc");
    check(cudaMalloc(&dScan, sizeof(int) * n), "malloc");
    check(cudaMalloc(&dKept, sizeof(int) * n), "malloc");
    check(cudaMalloc(&dCount, sizeof(int) * warps), "malloc");
    check(cudaMemcpy(dIn, h.data(), sizeof(int) * n, cudaMemcpyHostToDevice), "copy in");
    warpTests<<<n / threads, threads>>>(dIn, dSums, dAll, dScan, dKept, dCount);
    check(cudaGetLastError(), "warpTests");
    std::vector<int> sums(warps), all(n), scan(n), kept(n), count(warps);
    check(cudaMemcpy(sums.data(), dSums, sizeof(int) * warps, cudaMemcpyDeviceToHost), "copy");
    check(cudaMemcpy(all.data(), dAll, sizeof(int) * n, cudaMemcpyDeviceToHost), "copy");
    check(cudaMemcpy(scan.data(), dScan, sizeof(int) * n, cudaMemcpyDeviceToHost), "copy");
    check(cudaMemcpy(kept.data(), dKept, sizeof(int) * n, cudaMemcpyDeviceToHost), "copy");
    check(cudaMemcpy(count.data(), dCount, sizeof(int) * warps, cudaMemcpyDeviceToHost), "copy");
    int errors = 0;
    for (int w = 0; w < warps; ++w) {
        int run = 0, k = 0;
        for (int lane = 0; lane < 32; ++lane) {
            int v = h[w * 32 + lane];
            run += v;
            errors += scan[w * 32 + lane] != run;
            if (v % 3 == 0) { errors += kept[w * 32 + k] != v; ++k; }
        }
        errors += sums[w] != run;
        errors += count[w] != k;
        for (int lane = 0; lane < 32; ++lane) { errors += all[w * 32 + lane] != run; }
    }
    std::printf("warpTests: %d warps checked, %d errors\n", warps, errors);
    for (int* p : {dIn, dSums, dAll, dScan, dKept, dCount}) { check(cudaFree(p), "cudaFree"); }
    return 0;
}
