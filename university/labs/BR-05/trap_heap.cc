// trap_heap.cc - BR-05 Listing 9: trap 4 on bare metal, dynamic allocation in the control
// step. A "harmless" scratch buffer from new[] in every step. The OS305 firmware is built
// without any library (-nostdlib), so there is no heap: the link is EXPECTED to fail, and
// that failure is the lesson (run.sh records it).
#include "bm_time.h"
#include "board.h"
#include "loop_core.h"
#include "mcu_common.h"

namespace {

br05::Loop loop;

}  // namespace

extern "C" void SysTick_Handler()
{
    bm::ticks = bm::ticks + 1;
    int32_t* history = new int32_t[16];                  // a heap allocation every period
    history[0] = loop.step();
    board::setLeds(static_cast<uint32_t>(history[0]) >> 7);
    delete[] history;
}

int main()
{
    board::uartInit();
    bm::startTicks();
    while (bm::ticks < 10) {
        asm volatile("wfi");
    }
    board::exitEmulator(true);
}
