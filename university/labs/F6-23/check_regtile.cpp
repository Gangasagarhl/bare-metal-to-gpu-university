// F6-23 Listing 3: the register-tiled kernels, unchanged, on the CPU emulator.
#include "../F6-21/cuda_shim.hpp"
#define LAUNCH(kernel, grid, block, ...) launch(grid, block, kernel, __VA_ARGS__)
#include "gemm_regtile.cuh"
#include "launchers.cuh"
#include "../F6-21/emu_test.hpp"

int main()
{
    std::printf("register-tiled kernels on the CPU emulator:\n");
    int failed = emuTestAll("1d 64x64 TM8", launchRegTile<64, 64, 8, 8, 1, false>);
    failed += emuTestAll("2d 64x64 4x4", launchRegTile<64, 64, 8, 4, 4, false>);
    failed += emuTestAll("2d 64x64 4x4 v4", launchRegTile<64, 64, 8, 4, 4, true>);
    failed += emuTestAll("2d 128x128 8x8 v4", launchRegTile<128, 128, 8, 8, 8, true>);
    std::printf("%s\n", failed ? "SOME SHAPES FAILED" : "all shapes ok");
    return failed ? 1 : 0;
}
