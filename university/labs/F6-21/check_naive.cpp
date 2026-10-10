// F6-21 Listing 3: run sgemmNaive, unchanged, on the CPU with cuda_shim.hpp and check it.
#include "cuda_shim.hpp"
#include "gemm_naive.cuh"
#include "emu_test.hpp"

int main()
{
    std::printf("sgemmNaive on the CPU emulator (16 x 16 threads per block):\n");
    const int failed = emuTestAll("naive", [](const float* A, const float* B, float* C, int M,
                                              int N, int K) {
        const dim3 block(16, 16);
        const dim3 grid((N + 15) / 16, (M + 15) / 16);
        launch(grid, block, sgemmNaive, A, B, C, M, N, K);
    });
    std::printf("%s\n", failed ? "SOME SHAPES FAILED" : "all shapes ok");
    return failed ? 1 : 0;
}
