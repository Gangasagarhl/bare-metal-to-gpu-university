// bm_irq.cc - BR-05 Listing 6: world 1b, bare metal with the control step in the timer
// interrupt handler. main() does only the housekeeping; the handler preempts it every
// period. Same control code, same housekeeping, different structure.
#include "bm_time.h"
#include "board.h"
#include "loop_core.h"
#include "mcu_common.h"

namespace {

br05::Loop loop;
mcu::Stats stats;
volatile bool finished = false;

}  // namespace

extern "C" void SysTick_Handler()
{
    const uint32_t k = bm::ticks + 1;
    bm::ticks = k;
    if (k > mcu::kSteps) {
        finished = true;
        return;
    }
    const int32_t u = loop.step();                       // sense, compute, actuate
    board::setLeds(static_cast<uint32_t>(u) >> 7);
    stats.add(k, bm::now() - k * mcu::kClocksPerPeriod);
}

int main()
{
    board::uartInit();
    board::print("BR-05 world 1b: bare metal, control step in the timer interrupt\n");
    mcu::calibrate();
    bm::startTicks();
    uint32_t round = 0;
    while (!finished) {
        if (round % 10 == 0) {
            mcu::work(15);                               // housekeeping: 1.5 periods of work
        } else {
            asm volatile("wfi");
        }
        round = round + 1;
    }
    board::reg(board::kSystCsr) = 0;                     // stop the tick before reporting
    stats.print("bare-metal-interrupt", loop.checksum);
    board::exitEmulator(true);
}
