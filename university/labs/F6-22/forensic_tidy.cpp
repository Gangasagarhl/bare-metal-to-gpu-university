// F6-22 forensic evidence: the "tidy" kernel on the CPU emulator, shapes in order.
// Output is unbuffered so that every finished line survives if the run is stopped.
#include "../F6-21/cuda_shim.hpp"
#include "tiled_early_return.cuh"
#include "../F6-21/emu_test.hpp"

int main()
{
    std::setvbuf(stdout, nullptr, _IONBF, 0);
    std::printf("sgemmTiledTidy<16> on the CPU emulator:\n");
    const int shapes[][3] = {{64, 64, 64}, {128, 32, 48}, {127, 129, 65}};
    for (const auto& s : shapes) {
        std::printf("starting M=%d N=%d K=%d ...\n", s[0], s[1], s[2]);
        emuTestShape("tidy<16>", [](const float* A, const float* B, float* C, int M, int N, int K) {
            launch(dim3((N + 15) / 16, (M + 15) / 16), dim3(16, 16), sgemmTiledTidy<16>, A, B, C,
                   M, N, K);
        }, s[0], s[1], s[2]);
    }
    return 0;
}
