// F6-19 Listing 1: matrix transpose, five kernels (curriculum milestone E5), checked on a
// non-square, non-multiple-of-32 shape and timed against a plain copy (median of 20 runs).
//   copyTile        - the bandwidth reference: same tiles, no transpose
//   naive           - reads rows (coalesced), writes columns (strided)
//   tiledNoPad      - through a 32 x 32 shared tile: both global accesses coalesced, bank conflicts
//   tiledPad        - the same with a 32 x 33 tile: no bank conflicts
//   tiledSwizzle    - a 32 x 32 tile with the column index XOR-ed with the row: no conflicts, no padding
// Built for real; untested on hardware.
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <vector>
#include <cuda_runtime.h>

constexpr int TILE = 32;
constexpr int ROWS = 8;     // 32 x 8 threads per block; each thread moves 4 elements

__global__ void copyTile(const float* in, float* out, int width, int height)
{
    int x = blockIdx.x * TILE + threadIdx.x;
    int y = blockIdx.y * TILE + threadIdx.y;
    for (int j = 0; j < TILE; j += ROWS) {
        if (x < width && y + j < height) { out[(y + j) * width + x] = in[(y + j) * width + x]; }
    }
}

__global__ void naive(const float* in, float* out, int width, int height)
{
    int x = blockIdx.x * TILE + threadIdx.x;
    int y = blockIdx.y * TILE + threadIdx.y;
    for (int j = 0; j < TILE; j += ROWS) {
        if (x < width && y + j < height) { out[x * height + (y + j)] = in[(y + j) * width + x]; }
    }
}

// MODE 0: pitch 32; MODE 1: pitch 33 (padding); MODE 2: pitch 32 with XOR swizzle
template <int MODE>
__device__ void tiled(const float* in, float* out, int width, int height)
{
    __shared__ float tile[TILE][MODE == 1 ? TILE + 1 : TILE];
    int x = blockIdx.x * TILE + threadIdx.x;
    int y = blockIdx.y * TILE + threadIdx.y;
    for (int j = 0; j < TILE; j += ROWS) {
        int r = threadIdx.y + j;
        int c = (MODE == 2) ? (threadIdx.x ^ r) : threadIdx.x;      // swizzled column
        if (x < width && y + j < height) { tile[r][c] = in[(y + j) * width + x]; }
    }
    __syncthreads();
    x = blockIdx.y * TILE + threadIdx.x;                             // swap the block coordinates
    y = blockIdx.x * TILE + threadIdx.y;
    for (int j = 0; j < TILE; j += ROWS) {
        int r = threadIdx.x;                                         // read a column of the tile
        int c = (MODE == 2) ? ((threadIdx.y + j) ^ r) : (threadIdx.y + j);
        if (x < height && y + j < width) { out[(y + j) * height + x] = tile[r][c]; }
    }
}

__global__ void tiledNoPad(const float* in, float* out, int w, int h) { tiled<0>(in, out, w, h); }
__global__ void tiledPad(const float* in, float* out, int w, int h) { tiled<1>(in, out, w, h); }
__global__ void tiledSwizzle(const float* in, float* out, int w, int h) { tiled<2>(in, out, w, h); }

static void check(cudaError_t err, const char* what)
{
    if (err != cudaSuccess) {
        std::fprintf(stderr, "%s failed: %s (%s)\n", what, cudaGetErrorName(err), cudaGetErrorString(err));
        std::exit(EXIT_FAILURE);
    }
}

using Kernel = void (*)(const float*, float*, int, int);

float timeKernel(Kernel k, const float* dIn, float* dOut, int w, int h)
{
    dim3 block(TILE, ROWS), grid((w + TILE - 1) / TILE, (h + TILE - 1) / TILE);
    cudaEvent_t start, stop;
    check(cudaEventCreate(&start), "event");
    check(cudaEventCreate(&stop), "event");
    k<<<grid, block>>>(dIn, dOut, w, h);                              // warm-up
    check(cudaGetLastError(), "launch");
    std::vector<float> ms(20);
    for (float& m : ms) {
        check(cudaEventRecord(start), "record");
        k<<<grid, block>>>(dIn, dOut, w, h);
        check(cudaEventRecord(stop), "record");
        check(cudaEventSynchronize(stop), "sync");
        check(cudaEventElapsedTime(&m, start, stop), "elapsed");
    }
    std::sort(ms.begin(), ms.end());
    check(cudaEventDestroy(start), "event");
    check(cudaEventDestroy(stop), "event");
    return (ms[9] + ms[10]) / 2;
}

int main()
{
    int dev = 0;
    check(cudaGetDevice(&dev), "cudaGetDevice");
    const Kernel kernels[] = {copyTile, naive, tiledNoPad, tiledPad, tiledSwizzle};
    const char* names[] = {"copyTile", "naive", "tiledNoPad", "tiledPad", "tiledSwizzle"};
    const int shapes[][2] = {{1000, 700}, {4096, 4096}};               // width x height
    for (const auto& s : shapes) {
        const int w = s[0], h = s[1];
        const size_t count = static_cast<size_t>(w) * h;
        std::vector<float> hIn(count), hOut(count);
        for (size_t k = 0; k < count; ++k) { hIn[k] = static_cast<float>(k % 10007); }
        float *dIn, *dOut;
        check(cudaMalloc(&dIn, count * sizeof(float)), "malloc");
        check(cudaMalloc(&dOut, count * sizeof(float)), "malloc");
        check(cudaMemcpy(dIn, hIn.data(), count * sizeof(float), cudaMemcpyHostToDevice), "copy in");
        float copyMs = 0.0f;
        for (int v = 0; v < 5; ++v) {
            float ms = timeKernel(kernels[v], dIn, dOut, w, h);
            if (v == 0) { copyMs = ms; }
            check(cudaMemcpy(hOut.data(), dOut, count * sizeof(float), cudaMemcpyDeviceToHost), "copy out");
            long errors = 0;
            for (int r = 0; r < h; ++r) {
                for (int c = 0; c < w; ++c) {
                    float want = hIn[static_cast<size_t>(r) * w + c];
                    float got = (v == 0) ? hOut[static_cast<size_t>(r) * w + c] : hOut[static_cast<size_t>(c) * h + r];
                    errors += (got != want);
                }
            }
            double gbs = 2.0 * count * sizeof(float) / (ms * 1.0e6);
            std::printf("%4d x %-4d %-13s %ld errors, %.3f ms, %.1f GB/s, %.0f %% of copy\n", w, h, names[v],
                        errors, ms, gbs, 100.0 * copyMs / ms);
        }
        check(cudaFree(dIn), "free");
        check(cudaFree(dOut), "free");
    }
    return 0;
}
