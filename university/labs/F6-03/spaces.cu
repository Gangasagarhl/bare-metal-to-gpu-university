// F6-03 Listing 1: one kernel that touches every memory space of this chapter.
// out[i] = c0*in[i] + c1*in[right neighbour in the block] + c2*(sum of a small private table)
#include <cstdio>
#include <cstdlib>
#include <vector>
#include <cuda_runtime.h>

constexpr int kThreads = 256;           // threads per block (also the shared tile size)
__constant__ float coeff[3];            // constant memory: the same three numbers for every thread

__global__ void spaces(const float* in, float* out, int n, int pick)
{
    __shared__ float tile[kThreads];    // shared memory: one tile per block
    float table[8];                     // a per-thread array (registers or local memory)
    int t = threadIdx.x;                // t and i live in registers
    int i = blockIdx.x * blockDim.x + t;

    float x = (i < n) ? in[i] : 0.0f;   // global memory read
    tile[t] = x;
    __syncthreads();                    // wait until the whole block has written its tile
    float right = tile[(t + 1) % kThreads];

    for (int k = 0; k < 8; ++k) {
        table[k] = x * k;
    }
    float picked = table[pick & 7];     // index known only at run time

    if (i < n) {
        out[i] = coeff[0] * x + coeff[1] * right + coeff[2] * picked;  // global memory write
    }
}

static void check(cudaError_t err, const char* what)
{
    if (err != cudaSuccess) {
        std::fprintf(stderr, "%s failed: %s (%s)\n", what, cudaGetErrorName(err),
                     cudaGetErrorString(err));
        std::exit(EXIT_FAILURE);
    }
}

int main()
{
    const int n = 1 << 16;               // a multiple of kThreads: every block is full
    const std::size_t bytes = n * sizeof(float);
    std::vector<float> hIn(n), hOut(n);
    for (int i = 0; i < n; ++i) { hIn[i] = static_cast<float>(i % 100); }
    const float hCoeff[3] = {1.0f, 2.0f, 0.5f};

    float* dIn = nullptr;
    float* dOut = nullptr;
    check(cudaMalloc(&dIn, bytes), "cudaMalloc in");
    check(cudaMalloc(&dOut, bytes), "cudaMalloc out");
    check(cudaMemcpy(dIn, hIn.data(), bytes, cudaMemcpyHostToDevice), "copy in");
    check(cudaMemcpyToSymbol(coeff, hCoeff, sizeof(hCoeff)), "copy coefficients");
    spaces<<<n / kThreads, kThreads>>>(dIn, dOut, n, 3);
    check(cudaGetLastError(), "kernel launch");
    check(cudaMemcpy(hOut.data(), dOut, bytes, cudaMemcpyDeviceToHost), "copy out");

    int errors = 0;
    for (int i = 0; i < n; ++i) {
        const int block = i / kThreads;
        const int right = block * kThreads + (i % kThreads + 1) % kThreads;
        const float want = hCoeff[0] * hIn[i] + hCoeff[1] * hIn[right] + hCoeff[2] * (hIn[i] * 3);
        if (hOut[i] != want) { ++errors; }
    }
    std::printf("checked %d elements, %d errors\n", n, errors);
    check(cudaFree(dIn), "cudaFree in");
    check(cudaFree(dOut), "cudaFree out");
    return errors == 0 ? 0 : 1;
}
