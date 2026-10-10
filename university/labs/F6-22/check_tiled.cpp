// F6-22 Listing 3: run sgemmTiled<8> and sgemmTiled<16>, unchanged, on the CPU emulator.
#include "../F6-21/cuda_shim.hpp"
#include "gemm_tiled.cuh"
#include "../F6-21/emu_test.hpp"

template <int TILE>
void launchTiled(const float* A, const float* B, float* C, int M, int N, int K)
{
    launch(dim3((N + TILE - 1) / TILE, (M + TILE - 1) / TILE), dim3(TILE, TILE), sgemmTiled<TILE>,
           A, B, C, M, N, K);
}

int main()
{
    std::printf("sgemmTiled on the CPU emulator:\n");
    int failed = emuTestAll("tiled<8>", launchTiled<8>);
    failed += emuTestAll("tiled<16>", launchTiled<16>);
    std::printf("%s\n", failed ? "SOME SHAPES FAILED" : "all shapes ok");
    return failed ? 1 : 0;
}
