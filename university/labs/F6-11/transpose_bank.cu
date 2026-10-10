// F6-11 forensic evidence: the transpose kernel from the scenario (transposeNoPad) and the fix
// (transposePad). Same code except one number: the tile pitch. 32 x 8 threads per block,
// each thread moves TILE / ROWS = 4 elements. Built for real; untested on hardware.
#include <cstdio>
#include <cstdlib>
#include <vector>
#include <cuda_runtime.h>

constexpr int TILE = 32;
constexpr int ROWS = 8;

template <int PITCH>
__device__ void transposeTile(const float* in, float* out, int width, int height)
{
    __shared__ float tile[TILE][PITCH];
    int x = blockIdx.x * TILE + threadIdx.x;
    int y = blockIdx.y * TILE + threadIdx.y;
    for (int j = 0; j < TILE; j += ROWS) {
        if (x < width && y + j < height) {
            tile[threadIdx.y + j][threadIdx.x] = in[(y + j) * width + x];   // row write to the tile
        }
    }
    __syncthreads();
    x = blockIdx.y * TILE + threadIdx.x;
    y = blockIdx.x * TILE + threadIdx.y;
    for (int j = 0; j < TILE; j += ROWS) {
        if (x < height && y + j < width) {
            out[(y + j) * height + x] = tile[threadIdx.x][threadIdx.y + j]; // column read of the tile
        }
    }
}

__global__ void transposeNoPad(const float* in, float* out, int width, int height)
{
    transposeTile<TILE>(in, out, width, height);
}

__global__ void transposePad(const float* in, float* out, int width, int height)
{
    transposeTile<TILE + 1>(in, out, width, height);
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
    const int width = 4096, height = 4096;
    const std::size_t count = static_cast<std::size_t>(width) * height;
    std::vector<float> hIn(count), hOut(count);
    for (std::size_t k = 0; k < count; ++k) { hIn[k] = static_cast<float>(k % 9973); }
    float* dIn = nullptr;
    float* dOut = nullptr;
    check(cudaMalloc(&dIn, count * sizeof(float)), "cudaMalloc in");
    check(cudaMalloc(&dOut, count * sizeof(float)), "cudaMalloc out");
    check(cudaMemcpy(dIn, hIn.data(), count * sizeof(float), cudaMemcpyHostToDevice), "copy in");
    dim3 block(TILE, ROWS);
    dim3 grid((width + TILE - 1) / TILE, (height + TILE - 1) / TILE);
    for (int version = 0; version < 2; ++version) {
        if (version == 0) {
            transposeNoPad<<<grid, block>>>(dIn, dOut, width, height);
        } else {
            transposePad<<<grid, block>>>(dIn, dOut, width, height);
        }
        check(cudaGetLastError(), "launch");
        check(cudaMemcpy(hOut.data(), dOut, count * sizeof(float), cudaMemcpyDeviceToHost), "copy out");
        long errors = 0;
        for (int r = 0; r < height; ++r) {
            for (int c = 0; c < width; ++c) {
                if (hOut[static_cast<std::size_t>(c) * height + r] != hIn[static_cast<std::size_t>(r) * width + c]) {
                    ++errors;
                }
            }
        }
        std::printf("%s: %ld errors\n", version == 0 ? "transposeNoPad" : "transposePad", errors);
    }
    check(cudaFree(dIn), "cudaFree in");
    check(cudaFree(dOut), "cudaFree out");
    return 0;
}
