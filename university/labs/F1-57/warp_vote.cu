// F1-57 Listing 2: a divergent branch and a warp vote in CUDA.
// Compiled for real (also to PTX and SASS by run.sh); untested on hardware.
#include <cstdio>
#include <cstdlib>
#include <vector>
#include <cuda_runtime.h>

__global__ void collatzStep(const int* in, int* out, int n)
{
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i >= n) {
        return;
    }
    int v = in[i];
    if (v & 1) {                       // lanes of one warp may disagree here
        v = v * 3 + 1;
    } else {
        v = v / 2;
    }
    unsigned votes = __ballot_sync(0xffffffffu, v > 10);   // one bit per lane
    out[i] = v + __popc(votes) + warpSize;                  // warpSize: built-in variable
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
    const int n = 1024;
    std::vector<int> hIn(n), hOut(n);
    for (int i = 0; i < n; ++i) { hIn[i] = i; }
    int* dIn = nullptr;
    int* dOut = nullptr;
    check(cudaMalloc(&dIn, n * sizeof(int)), "cudaMalloc in");
    check(cudaMalloc(&dOut, n * sizeof(int)), "cudaMalloc out");
    check(cudaMemcpy(dIn, hIn.data(), n * sizeof(int), cudaMemcpyHostToDevice), "copy in");
    collatzStep<<<n / 256, 256>>>(dIn, dOut, n);
    check(cudaGetLastError(), "kernel launch");
    check(cudaMemcpy(hOut.data(), dOut, n * sizeof(int), cudaMemcpyDeviceToHost), "copy out");
    std::printf("out[0..3] = %d %d %d %d\n", hOut[0], hOut[1], hOut[2], hOut[3]);
    check(cudaFree(dIn), "cudaFree in");
    check(cudaFree(dOut), "cudaFree out");
    return 0;
}
