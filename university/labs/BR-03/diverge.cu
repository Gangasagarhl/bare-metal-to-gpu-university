// BR-03 Listing 6: a weighted histogram step with a data-dependent branch. Bytes >= 200
// need extra work (a loop of `extra` steps); the others do not. On a CPU each thread pays
// only for its own bytes; in a warp, the lanes share one instruction stream.
// Compiled only in this build (its machine code is read by run.sh); untested on hardware.
#include <cstdio>
#include <cuda_runtime.h>

__global__ void weigh(const unsigned char* in, size_t n, unsigned* bins, int extra)
{
    size_t i = static_cast<size_t>(blockIdx.x) * blockDim.x + threadIdx.x;
    if (i >= n) { return; }
    unsigned v = in[i];
    unsigned w = 1;
    if (v >= 200) {                                   // rare, expensive path
        for (int k = 0; k < extra; ++k) { w = w * 1664525u + 1013904223u; }
        w = (w >> 28) + 1;
    }
    atomicAdd(&bins[v], w);                           // both paths meet here again
}

int main()
{
    int count = 0;
    cudaError_t err = cudaGetDeviceCount(&count);
    std::printf("cudaGetDeviceCount: %s, %d device(s); this listing is read as machine code (run.sh)\n",
                cudaGetErrorName(err), count);
    return 0;
}
