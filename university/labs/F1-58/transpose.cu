// F1-58 Listing 1: a tiled matrix transpose that stages data in shared memory.
// TILE is a tile edge chosen by us (not a hardware constant); the +1 column is padding.
// Built for real and compiled to SASS by run.sh; untested on hardware.
#include <cstdio>
#include <cstdlib>
#include <vector>
#include <cuda_runtime.h>

constexpr int TILE = 32;

__global__ void transposeTiled(const float* in, float* out, int rows, int cols)
{
    __shared__ float tile[TILE][TILE + 1];          // on-chip, shared by the block
    int c = blockIdx.x * TILE + threadIdx.x;
    int r = blockIdx.y * TILE + threadIdx.y;
    if (r < rows && c < cols) {
        tile[threadIdx.y][threadIdx.x] = in[r * cols + c];   // read a row segment
    }
    __syncthreads();                                 // whole tile loaded
    int oc = blockIdx.y * TILE + threadIdx.x;        // swap block coordinates
    int orow = blockIdx.x * TILE + threadIdx.y;
    if (orow < cols && oc < rows) {
        out[orow * rows + oc] = tile[threadIdx.x][threadIdx.y];  // column of the tile
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
    const int rows = 1000, cols = 700;
    const std::size_t bytes = static_cast<std::size_t>(rows) * cols * sizeof(float);
    std::vector<float> hIn(static_cast<std::size_t>(rows) * cols), hOut(hIn.size());
    for (std::size_t k = 0; k < hIn.size(); ++k) { hIn[k] = static_cast<float>(k); }
    float* dIn = nullptr;
    float* dOut = nullptr;
    check(cudaMalloc(&dIn, bytes), "cudaMalloc in");
    check(cudaMalloc(&dOut, bytes), "cudaMalloc out");
    check(cudaMemcpy(dIn, hIn.data(), bytes, cudaMemcpyHostToDevice), "copy in");
    dim3 block(TILE, TILE);
    dim3 grid((cols + TILE - 1) / TILE, (rows + TILE - 1) / TILE);
    transposeTiled<<<grid, block>>>(dIn, dOut, rows, cols);
    check(cudaGetLastError(), "kernel launch");
    check(cudaMemcpy(hOut.data(), dOut, bytes, cudaMemcpyDeviceToHost), "copy out");
    int errors = 0;
    for (int r = 0; r < rows; ++r) {
        for (int c = 0; c < cols; ++c) {
            if (hOut[static_cast<std::size_t>(c) * rows + r] != hIn[static_cast<std::size_t>(r) * cols + c]) { ++errors; }
        }
    }
    std::printf("checked %d elements, %d errors\n", rows * cols, errors);
    check(cudaFree(dIn), "cudaFree in");
    check(cudaFree(dOut), "cudaFree out");
    return errors == 0 ? 0 : 1;
}
