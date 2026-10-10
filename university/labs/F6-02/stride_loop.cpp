// F6-02 Listing 3: which elements each thread handles in a grid-stride loop
// (a CPU model of the loop in the comment below; 2 blocks of 4 threads, n = 21).
//
//   for (int i = blockIdx.x * blockDim.x + threadIdx.x; i < n; i += blockDim.x * gridDim.x)
#include <cstdio>

int main()
{
    const int n = 21;
    const int blockDim = 4;
    const int gridDim = 2;
    const int stride = blockDim * gridDim;
    for (int block = 0; block < gridDim; ++block) {
        for (int thread = 0; thread < blockDim; ++thread) {
            std::printf("block %d thread %d:", block, thread);
            for (int i = block * blockDim + thread; i < n; i += stride) {
                std::printf(" %2d", i);
            }
            std::printf("\n");
        }
    }
    std::printf("stride = blockDim.x * gridDim.x = %d\n", stride);
    return 0;
}
