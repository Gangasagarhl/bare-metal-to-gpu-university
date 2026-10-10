// F6-24 forensic evidence: the colleague's kernel on the CPU emulator, where an
// asynchronous copy lands only when its thread waits for it.
#include "../F6-21/cuda_shim.hpp"
#define LAUNCH(kernel, grid, block, ...) launch(grid, block, kernel, __VA_ARGS__)
#include "../F6-23/gemm_regtile.cuh"
#include "dbuf_fast.cuh"
#include "../F6-21/emu_test.hpp"

int main()
{
    std::printf("sgemmDoubleBufferFast on the CPU emulator:\n");
    const int failed = emuTestAll("fast 64x64/4x4", launchDoubleBufferFast<64, 64, 8, 4, 4>);
    std::printf("%s\n", failed ? "SOME SHAPES FAILED" : "all shapes ok");
    return 0;
}
