// F6-16 Listing 3: a device-wide inclusive scan in CUDA (reduce-then-scan, three kernels) built from
// a block scan made of warp shuffles, checked against cub::DeviceScan::InclusiveSum (CUB 2.0.1).
// Built for real; untested on hardware.
#include <cstdio>
#include <cstdlib>
#include <numeric>
#include <vector>
#include <cub/cub.cuh>
#include <cuda_runtime.h>

constexpr int BLOCK = 256;                  // one tile = one block = 256 elements

__device__ int blockInclusiveScan(int v)
{
    __shared__ int warpTotals[BLOCK / 32];
    int lane = threadIdx.x % 32, warp = threadIdx.x / 32;
    for (int d = 1; d < 32; d *= 2) {                       // scan inside each warp
        int up = __shfl_up_sync(0xffffffffu, v, d);
        if (lane >= d) { v += up; }
    }
    if (lane == 31) { warpTotals[warp] = v; }
    __syncthreads();
    if (warp == 0) {                                        // scan the warp totals
        int t = (lane < BLOCK / 32) ? warpTotals[lane] : 0;
        for (int d = 1; d < 32; d *= 2) {
            int up = __shfl_up_sync(0xffffffffu, t, d);
            if (lane >= d) { t += up; }
        }
        if (lane < BLOCK / 32) { warpTotals[lane] = t; }
    }
    __syncthreads();
    if (warp > 0) { v += warpTotals[warp - 1]; }            // add the totals of earlier warps
    return v;
}

__global__ void scanTiles(const int* in, int* out, int* tileSums, int n)
{
    int i = blockIdx.x * BLOCK + threadIdx.x;
    int v = blockInclusiveScan(i < n ? in[i] : 0);
    if (i < n) { out[i] = v; }
    if (threadIdx.x == BLOCK - 1) { tileSums[blockIdx.x] = v; }
}

__global__ void addOffsets(int* out, const int* scannedSums, int n)
{
    int i = blockIdx.x * BLOCK + threadIdx.x;
    if (blockIdx.x > 0 && i < n) { out[i] += scannedSums[blockIdx.x - 1]; }
}

static void check(cudaError_t err, const char* what)
{
    if (err != cudaSuccess) {
        std::fprintf(stderr, "%s failed: %s (%s)\n", what, cudaGetErrorName(err), cudaGetErrorString(err));
        std::exit(EXIT_FAILURE);
    }
}

// Scans d[0..n) in place-out to dOut, recursing on the tile sums.
void deviceScan(const int* dIn, int* dOut, int n)
{
    int tiles = (n + BLOCK - 1) / BLOCK;
    int* dSums = nullptr;
    check(cudaMalloc(&dSums, sizeof(int) * tiles), "malloc sums");
    scanTiles<<<tiles, BLOCK>>>(dIn, dOut, dSums, n);
    check(cudaGetLastError(), "scanTiles");
    if (tiles > 1) {
        int* dScanned = nullptr;
        check(cudaMalloc(&dScanned, sizeof(int) * tiles), "malloc scanned");
        deviceScan(dSums, dScanned, tiles);                 // recursion: scan of the tile sums
        addOffsets<<<tiles, BLOCK>>>(dOut, dScanned, n);
        check(cudaGetLastError(), "addOffsets");
        check(cudaFree(dScanned), "free scanned");
    }
    check(cudaFree(dSums), "free sums");
}

int main()
{
    int dev = 0;
    check(cudaGetDevice(&dev), "cudaGetDevice");
    const int sizes[] = {1, 255, 256, 257, (1 << 20) - 1, (1 << 20) + 1, 10000019};
    int failures = 0;
    for (int n : sizes) {
        std::vector<int> h(n), ref(n), mine(n), cubOut(n);
        for (int i = 0; i < n; ++i) { h[i] = (i * 13) % 7; }
        std::inclusive_scan(h.begin(), h.end(), ref.begin());
        int *dIn, *dOut, *dCub;
        check(cudaMalloc(&dIn, sizeof(int) * n), "malloc");
        check(cudaMalloc(&dOut, sizeof(int) * n), "malloc");
        check(cudaMalloc(&dCub, sizeof(int) * n), "malloc");
        check(cudaMemcpy(dIn, h.data(), sizeof(int) * n, cudaMemcpyHostToDevice), "copy");
        deviceScan(dIn, dOut, n);
        void* dTemp = nullptr;
        size_t tempBytes = 0;
        check(cub::DeviceScan::InclusiveSum(dTemp, tempBytes, dIn, dCub, n), "CUB size query");
        check(cudaMalloc(&dTemp, tempBytes), "malloc temp");
        check(cub::DeviceScan::InclusiveSum(dTemp, tempBytes, dIn, dCub, n), "CUB InclusiveSum");
        check(cudaMemcpy(mine.data(), dOut, sizeof(int) * n, cudaMemcpyDeviceToHost), "copy");
        check(cudaMemcpy(cubOut.data(), dCub, sizeof(int) * n, cudaMemcpyDeviceToHost), "copy");
        bool ok = mine == ref && cubOut == ref;
        failures += !ok;
        std::printf("n = %-9d mine %s, CUB %s\n", n, mine == ref ? "pass" : "FAIL", cubOut == ref ? "pass" : "FAIL");
        for (void* p : {static_cast<void*>(dIn), static_cast<void*>(dOut), static_cast<void*>(dCub), dTemp}) {
            check(cudaFree(p), "free");
        }
    }
    return failures == 0 ? 0 : 1;
}
