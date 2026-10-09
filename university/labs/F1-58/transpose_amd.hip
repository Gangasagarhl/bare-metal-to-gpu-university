// F1-58 Listing 2: the same kernel in HIP; AMD documents call this memory the LDS.
// Only the kernel is shown; run.sh compiles it to gfx90a assembly. The host program
// is identical to Listing 1 with hip instead of cuda prefixes. Untested on hardware.
#include <cstdio>
#include <cstdlib>
#include <vector>
#include <hip/hip_runtime.h>

constexpr int TILE = 32;

__global__ void transposeTiled(const float* in, float* out, int rows, int cols)
{
    __shared__ float tile[TILE][TILE + 1];
    int c = blockIdx.x * TILE + threadIdx.x;
    int r = blockIdx.y * TILE + threadIdx.y;
    if (r < rows && c < cols) {
        tile[threadIdx.y][threadIdx.x] = in[r * cols + c];
    }
    __syncthreads();
    int oc = blockIdx.y * TILE + threadIdx.x;
    int orow = blockIdx.x * TILE + threadIdx.y;
    if (orow < cols && oc < rows) {
        out[orow * rows + oc] = tile[threadIdx.x][threadIdx.y];
    }
}

static void check(hipError_t err, const char* what)
{
    if (err != hipSuccess) {
        std::fprintf(stderr, "%s failed: %s (%s)\n", what, hipGetErrorName(err), hipGetErrorString(err));
        std::exit(EXIT_FAILURE);
    }
}

int main()
{
    const int rows = 1000, cols = 700;
    const std::size_t bytes = static_cast<std::size_t>(rows) * cols * sizeof(float);
    std::vector<float> hIn(static_cast<std::size_t>(rows) * cols, 1.0f), hOut(hIn.size());
    float* dIn = nullptr;
    float* dOut = nullptr;
    check(hipMalloc(&dIn, bytes), "hipMalloc in");
    check(hipMalloc(&dOut, bytes), "hipMalloc out");
    check(hipMemcpy(dIn, hIn.data(), bytes, hipMemcpyHostToDevice), "copy in");
    dim3 block(TILE, TILE);
    dim3 grid((cols + TILE - 1) / TILE, (rows + TILE - 1) / TILE);
    transposeTiled<<<grid, block>>>(dIn, dOut, rows, cols);
    check(hipGetLastError(), "kernel launch");
    check(hipMemcpy(hOut.data(), dOut, bytes, hipMemcpyDeviceToHost), "copy out");
    std::printf("copied back %zu elements\n", hOut.size());
    check(hipFree(dIn), "hipFree in");
    check(hipFree(dOut), "hipFree out");
    return 0;
}
