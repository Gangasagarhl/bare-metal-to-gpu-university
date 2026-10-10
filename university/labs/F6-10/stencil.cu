// F6-10 Listing 1: a 1-D stencil, out[i] = in[i-R] + ... + in[i+R], staged in shared memory.
// Two kernels: a static tile (size fixed at compile time) and a dynamic tile (size set at launch).
// Built for real; untested on hardware (the build container has no GPU).
#include <cstdio>
#include <cstdlib>
#include <vector>
#include <cuda_runtime.h>

constexpr int RADIUS = 3;     // chosen by us
constexpr int BLOCK = 256;    // threads per block, chosen by us

__global__ void stencilStatic(const int* in, int* out, int n)
{
    __shared__ int tile[BLOCK + 2 * RADIUS];       // block's slice plus a halo on each side
    int g = blockIdx.x * blockDim.x + threadIdx.x; // global index
    int s = threadIdx.x + RADIUS;                  // index inside the tile
    tile[s] = (g < n) ? in[g] : 0;
    if (threadIdx.x < RADIUS) {                    // the first RADIUS threads also load the halo
        int left = g - RADIUS;
        int right = g + BLOCK;
        tile[s - RADIUS] = (left >= 0 && left < n) ? in[left] : 0;
        tile[s + BLOCK] = (right < n) ? in[right] : 0;
    }
    __syncthreads();                               // every element of the tile is now written
    if (g < n) {
        int sum = 0;
        for (int k = -RADIUS; k <= RADIUS; ++k) {
            sum += tile[s + k];                    // 2R+1 reads from shared memory
        }
        out[g] = sum;
    }
}

__global__ void stencilDynamic(const int* in, int* out, int n, int radius)
{
    extern __shared__ int dtile[];                 // size given by the third launch parameter
    int g = blockIdx.x * blockDim.x + threadIdx.x;
    int s = threadIdx.x + radius;
    dtile[s] = (g < n) ? in[g] : 0;
    if (threadIdx.x < radius) {
        int left = g - radius;
        int right = g + static_cast<int>(blockDim.x);
        dtile[s - radius] = (left >= 0 && left < n) ? in[left] : 0;
        dtile[s + blockDim.x] = (right < n) ? in[right] : 0;
    }
    __syncthreads();
    if (g < n) {
        int sum = 0;
        for (int k = -radius; k <= radius; ++k) {
            sum += dtile[s + k];
        }
        out[g] = sum;
    }
}

static void check(cudaError_t err, const char* what)
{
    if (err != cudaSuccess) {
        std::fprintf(stderr, "%s failed: %s (%s)\n", what, cudaGetErrorName(err), cudaGetErrorString(err));
        std::exit(EXIT_FAILURE);
    }
}

static int countErrors(const std::vector<int>& got, const std::vector<int>& in, int radius)
{
    const int n = static_cast<int>(in.size());
    int errors = 0;
    for (int i = 0; i < n; ++i) {
        int ref = 0;
        for (int k = -radius; k <= radius; ++k) {
            if (i + k >= 0 && i + k < n) { ref += in[i + k]; }
        }
        if (got[i] != ref) { ++errors; }
    }
    return errors;
}

int main()
{
    const int n = 1000003;                          // not a multiple of BLOCK on purpose
    const std::size_t bytes = static_cast<std::size_t>(n) * sizeof(int);
    std::vector<int> hIn(n), hOut(n);
    for (int i = 0; i < n; ++i) { hIn[i] = (i * 7) % 11; }
    int* dIn = nullptr;
    int* dOut = nullptr;
    check(cudaMalloc(&dIn, bytes), "cudaMalloc in");
    check(cudaMalloc(&dOut, bytes), "cudaMalloc out");
    check(cudaMemcpy(dIn, hIn.data(), bytes, cudaMemcpyHostToDevice), "copy in");
    const int blocks = (n + BLOCK - 1) / BLOCK;

    stencilStatic<<<blocks, BLOCK>>>(dIn, dOut, n);
    check(cudaGetLastError(), "launch stencilStatic");
    check(cudaMemcpy(hOut.data(), dOut, bytes, cudaMemcpyDeviceToHost), "copy out (static)");
    std::printf("static  tile, radius %d: %d errors in %d outputs\n", RADIUS, countErrors(hOut, hIn, RADIUS), n);

    const int radius = 8;
    const std::size_t smem = (BLOCK + 2 * radius) * sizeof(int);
    stencilDynamic<<<blocks, BLOCK, smem>>>(dIn, dOut, n, radius);
    check(cudaGetLastError(), "launch stencilDynamic");
    check(cudaMemcpy(hOut.data(), dOut, bytes, cudaMemcpyDeviceToHost), "copy out (dynamic)");
    std::printf("dynamic tile, radius %d: %d errors in %d outputs (%zu bytes of shared memory per block)\n",
                radius, countErrors(hOut, hIn, radius), n, smem);

    check(cudaFree(dIn), "cudaFree in");
    check(cudaFree(dOut), "cudaFree out");
    return 0;
}
