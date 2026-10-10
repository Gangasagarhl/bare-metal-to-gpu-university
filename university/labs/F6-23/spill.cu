// F6-23 forensic: the 128 x 128 / 8 x 8 kernel with a colleague's "occupancy fix":
// __launch_bounds__(256, 4) asks the compiler for at least 4 resident blocks of 256
// threads per SM. Compiled only (run.sh reads its compiler report and SASS).
#define LAUNCH(kernel, grid, block, ...) kernel<<<grid, block>>>(__VA_ARGS__)
#include "gemm_regtile.cuh"

template __global__ void sgemmRegTile<128, 128, 8, 8, 8, true, 1>(const float*, const float*,
                                                                 float*, int, int, int);
template __global__ void sgemmRegTile<128, 128, 8, 8, 8, true, 4>(const float*, const float*,
                                                                 float*, int, int, int);

int main()
{
    return 0;
}
