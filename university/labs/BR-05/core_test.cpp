// core_test.cpp - BR-05 Listing 2: the control step tested on the PC before it goes anywhere
// else (built by run_lab.sh with sanitizers). Prints the trajectory every 100 steps and the
// checksum that the bare-metal, RTOS and Linux runs must reproduce exactly.
#include <cstdint>
#include <cstdio>

#include "loop_core.h"

int main()
{
    br05::Loop loop;
    constexpr int kSteps = 1000;                    // every world of the lab runs 1000 steps
    int32_t lastU = 0;
    for (int k = 1; k <= kSteps; ++k) {
        lastU = loop.step();
        if (k % 100 == 0) {
            std::printf("step %4d  temperature %6.2f degC  command %4d per mille\n", k,
                        loop.plant.tempMilliC / 1000.0, static_cast<int>(lastU));
        }
    }
    const bool settled = loop.plant.tempMilliC > 39800 && loop.plant.tempMilliC < 40200;
    std::printf("checksum after %d steps: 0x%08x\n", kSteps,
                static_cast<unsigned>(loop.checksum));
    std::printf("settled within 0.2 degC of 40 degC: %s\n", settled ? "yes" : "NO");
    return settled ? 0 : 1;
}
