// F7-15 Listing 1: a block-wide sum and exclusive scan that works for any wave width.
// Build: hipcc -std=c++17 -O2 --offload-arch=gfx90a block_scan.hip -o block_scan
#include <hip/hip_runtime.h>
#include <cstdio>
#include <cstdlib>
#include <vector>

constexpr int kBlock = 256;      // threads per block (a multiple of 32 and of 64)
constexpr int kMaxWaves = kBlock / 32;  // enough slots for the narrowest wave width

static void check(hipError_t e, const char* what)
{
    if (e != hipSuccess) {
        std::printf("%s failed: %s (%s)\n", what, hipGetErrorName(e), hipGetErrorString(e));
        std::exit(1);
    }
}

// Inclusive scan inside one wave: log2(warpSize) steps of __shfl_up.
__device__ int waveInclusiveScan(int v)
{
    const int lane = threadIdx.x % warpSize;
    for (int d = 1; d < warpSize; d *= 2) {
        int other = __shfl_up(v, d);
        if (lane >= d) {
            v += other;
        }
    }
    return v;
}

// One block scans kBlock elements; blockSums[b] receives the block's total.
__global__ void blockScan(const int* in, int* out, int* blockSums, int n)
{
    __shared__ int waveTotals[kMaxWaves];
    const int g = blockIdx.x * kBlock + threadIdx.x;
    const int lane = threadIdx.x % warpSize;
    const int wave = threadIdx.x / warpSize;
    const int numWaves = kBlock / warpSize;

    int x = (g < n) ? in[g] : 0;            // threads past the end add 0
    int incl = waveInclusiveScan(x);
    if (lane == warpSize - 1) {
        waveTotals[wave] = incl;            // last lane holds the wave's total
    }
    __syncthreads();
    if (wave == 0) {                        // wave 0 scans the wave totals
        int t = (lane < numWaves) ? waveTotals[lane] : 0;
        t = waveInclusiveScan(t);
        if (lane < numWaves) {
            waveTotals[lane] = t;
        }
    }
    __syncthreads();
    int waveOffset = (wave > 0) ? waveTotals[wave - 1] : 0;
    if (g < n) {
        out[g] = waveOffset + incl - x;     // inclusive minus own value = exclusive
    }
    if (threadIdx.x == kBlock - 1) {
        blockSums[blockIdx.x] = waveOffset + incl;
    }
}

int main()
{
    const int n = 1000003;                  // not a multiple of the block size
    const int blocks = (n + kBlock - 1) / kBlock;
    std::vector<int> hIn(n);
    for (int i = 0; i < n; ++i) {
        hIn[i] = (i * 7 + 3) % 11;
    }
    int* dIn = nullptr;
    int* dOut = nullptr;
    int* dSums = nullptr;
    check(hipMalloc(&dIn, n * sizeof(int)), "hipMalloc(dIn)");
    check(hipMalloc(&dOut, n * sizeof(int)), "hipMalloc(dOut)");
    check(hipMalloc(&dSums, blocks * sizeof(int)), "hipMalloc(dSums)");
    check(hipMemcpy(dIn, hIn.data(), n * sizeof(int), hipMemcpyHostToDevice), "hipMemcpy H2D");
    blockScan<<<blocks, kBlock>>>(dIn, dOut, dSums, n);
    check(hipGetLastError(), "launch blockScan");
    check(hipDeviceSynchronize(), "hipDeviceSynchronize");

    std::vector<int> hOut(n), hSums(blocks);
    check(hipMemcpy(hOut.data(), dOut, n * sizeof(int), hipMemcpyDeviceToHost), "hipMemcpy D2H");
    check(hipMemcpy(hSums.data(), dSums, blocks * sizeof(int), hipMemcpyDeviceToHost), "hipMemcpy D2H");
    // Second phase on the host: add the scanned block sums (a device scan does this on the GPU).
    long long errors = 0;
    int carry = 0;
    int running = 0;
    for (int b = 0; b < blocks; ++b) {
        for (int i = b * kBlock; i < n && i < (b + 1) * kBlock; ++i) {
            if (hOut[i] + carry != running) {
                ++errors;
            }
            running += hIn[i];
        }
        carry += hSums[b];
    }
    std::printf("n = %d, blocks = %d, errors = %lld\n", n, blocks, errors);
    check(hipFree(dIn), "hipFree");
    check(hipFree(dOut), "hipFree");
    check(hipFree(dSums), "hipFree");
    return errors == 0 ? 0 : 1;
}
