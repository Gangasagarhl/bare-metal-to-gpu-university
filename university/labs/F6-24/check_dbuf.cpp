// F6-24 Listing 3: the double-buffered kernel, unchanged, on the CPU emulator. The shim
// lands each asynchronous copy only when its thread waits for it (the latest moment the
// primitives allow), so a missing or misplaced wait shows up as wrong numbers.
#include "../F6-21/cuda_shim.hpp"
#define LAUNCH(kernel, grid, block, ...) launch(grid, block, kernel, __VA_ARGS__)
#include "../F6-23/gemm_regtile.cuh"
#include "gemm_dbuf.cuh"
#include "../F6-21/emu_test.hpp"

int main()
{
    std::printf("sgemmDoubleBuffer on the CPU emulator:\n");
    int failed = emuTestAll("dbuf 64x64/4x4", launchDoubleBuffer<64, 64, 8, 4, 4>);
    failed += emuTestAll("dbuf 128x128/8x8", launchDoubleBuffer<128, 128, 8, 8, 8>);
    std::printf("%s\n", failed ? "SOME SHAPES FAILED" : "all shapes ok");
    return failed ? 1 : 0;
}
