// bm_superloop.cc - BR-05 Listing 5: world 1a, bare metal with a superloop.
// One loop does everything: wait for the next period, run the control step, and every
// tenth period also run a 1.5-period housekeeping job (think: write a log page, compute
// statistics). The timer interrupt only counts periods; all work is in main().
#include "bm_time.h"
#include "board.h"
#include "loop_core.h"
#include "mcu_common.h"

extern "C" void SysTick_Handler()
{
    bm::ticks = bm::ticks + 1;
}

int main()
{
    board::uartInit();
    board::print("BR-05 world 1a: bare metal, superloop (control + housekeeping in one loop)\n");
    mcu::calibrate();
    br05::Loop loop;
    mcu::Stats stats;
    bm::startTicks();
    for (uint32_t k = 1; k <= mcu::kSteps; ++k) {
        while (bm::ticks < k) {                          // wait for release k (time k periods)
            asm volatile("wfi");
        }
        const int32_t u = loop.step();                   // sense, compute, actuate
        board::setLeds(static_cast<uint32_t>(u) >> 7);   // the "actuator" register write
        stats.add(k, bm::now() - k * mcu::kClocksPerPeriod);
        if (k % 10 == 0) {
            mcu::work(15);                               // housekeeping: 1.5 periods
        }
    }
    stats.print("bare-metal-superloop", loop.checksum);
    board::exitEmulator(true);
}
