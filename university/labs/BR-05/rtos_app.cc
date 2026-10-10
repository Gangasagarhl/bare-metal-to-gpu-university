// rtos_app.cc - BR-05 Listing 7: world 2, the same loop on an RTOS (uRTOS v2 from F3-40,
// a preview of F3-39 and F3-40). Two tasks instead of one loop:
//   control      (priority 2): sleep until the next period, run the control step;
//   housekeeping (priority 1): every 10 periods, 1.5 periods of work.
// BR05_LONG_CRITICAL=1 builds the forensic variant: housekeeping does 0.9 period of its
// work with interrupts disabled ("to protect the log buffer while copying it").
#include "board.h"
#include "loop_core.h"
#include "mcu_common.h"
#include "urtos.h"

#ifndef BR05_LONG_CRITICAL
#define BR05_LONG_CRITICAL 0
#endif

namespace {

br05::Loop loop;
mcu::Stats stats;

void controlTask()
{
    for (uint32_t k = 1; k <= mcu::kSteps; ++k) {
        os::sleepUntil(k);                               // block until release k
        const int32_t u = loop.step();                   // sense, compute, actuate
        board::setLeds(static_cast<uint32_t>(u) >> 7);
        stats.add(k, os::now() - k * mcu::kClocksPerPeriod);
    }
    os::sleepUntil(0xFFFFFFFFu);                         // done: never released again
}

void housekeepingTask()
{
    for (uint32_t release = 10;; release += 10) {
        os::sleepUntil(release);
        if (BR05_LONG_CRITICAL) {
            mcu::work(5);
            asm volatile("cpsid i" ::: "memory");        // no interrupts, no scheduler ...
            mcu::work(9);
            asm volatile("cpsie i" ::: "memory");        // ... for 0.9 period
            mcu::work(1);
        } else {
            mcu::work(15);                               // 1.5 periods, preemptible
        }
    }
}

// Print a time in periods with four decimals (25,000 clocks = 1 period, so 1 clock = 0.00004).
void printPeriods(uint32_t clocks)
{
    const uint32_t frac = (clocks % mcu::kClocksPerPeriod) * 2 / 5;   // ten-thousandths
    board::printDec(clocks / mcu::kClocksPerPeriod);
    board::putChar('.');
    for (uint32_t d = 1000; d >= 1; d /= 10) {
        board::putChar(static_cast<char>('0' + (frac / d) % 10));
    }
}

// The kernel's context-switch log between periods 9.9 and 12.1: when each task started.
void printSwitches()
{
    board::print("context switches between periods 9.9 and 12.1 (time in periods -> task now running):\n");
    for (uint32_t i = 0; i < os::switchCount; ++i) {
        const uint32_t t = os::switchLog[i].time;
        if (t >= 99 * mcu::kClocksPerPeriod / 10 && t <= 121 * mcu::kClocksPerPeriod / 10) {
            board::print("  ");
            printPeriods(t);
            board::print(" -> ");
            const char c = os::switchLog[i].to;
            board::print(c == 'C' ? "control" : c == 'H' ? "housekeeping" : "idle");
            board::print("\n");
        }
    }
}

void report()
{
    printSwitches();
    stats.print(BR05_LONG_CRITICAL ? "rtos-long-critical-section" : "rtos", loop.checksum);
    board::exitEmulator(true);
}

uint32_t controlStack[256], housekeepingStack[256];

}  // namespace

int main()
{
    board::uartInit();
    board::print(BR05_LONG_CRITICAL
                     ? "BR-05 world 2 (forensic build): RTOS, housekeeping with a long critical section\n"
                     : "BR-05 world 2: RTOS (uRTOS v2), control task above housekeeping task\n");
    mcu::calibrate();
    os::createTask(controlTask, 2, controlStack, 256, "control", 'C');
    os::createTask(housekeepingTask, 1, housekeepingStack, 256, "housekeeping", 'H');
    os::start(mcu::kReload, mcu::kSteps + 1, report);
}
