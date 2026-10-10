// BR-02 Listing 6: the same __shared__ keyword on both back ends. Each block reverses its
// own 256 values through a shared-memory tile (CUDA's name; AMD's documents call the
// memory behind it the LDS). Built by the lab runner for gfx90a; its ISA is read in run.sh.
#include <cstdio>
#include <cstdlib>
#include <vector>
#include <hip/hip_runtime.h>

constexpr int kTile = 256;

__global__ void reverseInBlock(const int* in, int* out)
{
    __shared__ int tile[kTile];
    const int t = threadIdx.x;
    const int base = blockIdx.x * kTile;
    tile[t] = in[base + t];
    __syncthreads();                                   // every write before any read
    out[base + t] = tile[kTile - 1 - t];
}

static void check(hipError_t err, const char* what)
{
    if (err != hipSuccess) {
        std::fprintf(stderr, "%s failed: %s\n", what, hipGetErrorName(err));
        std::exit(EXIT_FAILURE);
    }
}

int main()
{
    const int blocks = 4;
    const int n = blocks * kTile;                      // whole tiles only: no bounds check needed
    std::vector<int> hIn(n), hOut(n);
    for (int i = 0; i < n; ++i) { hIn[i] = i; }
    int* dIn = nullptr;
    int* dOut = nullptr;
    check(hipMalloc(&dIn, n * sizeof(int)), "hipMalloc in");
    check(hipMalloc(&dOut, n * sizeof(int)), "hipMalloc out");
    check(hipMemcpy(dIn, hIn.data(), n * sizeof(int), hipMemcpyHostToDevice), "copy in");
    reverseInBlock<<<blocks, kTile>>>(dIn, dOut);
    check(hipGetLastError(), "launch");
    check(hipMemcpy(hOut.data(), dOut, n * sizeof(int), hipMemcpyDeviceToHost), "copy out");
    int wrong = 0;
    for (int i = 0; i < n; ++i) {
        const int b = i / kTile;
        if (hOut[i] != b * kTile + (kTile - 1 - i % kTile)) { ++wrong; }
    }
    std::printf("%s (%d wrong)\n", wrong == 0 ? "PASS" : "FAIL", wrong);
    check(hipFree(dIn), "hipFree in");
    check(hipFree(dOut), "hipFree out");
    return wrong == 0 ? 0 : 1;
}
