// app.cc - F3-39 Listing 3: three periodic tasks on uRTOS with rate-monotonic priorities
// (shorter period = higher priority). Each job burns a known amount of processor time;
// the kernel's switch log and each task's measured worst response time are printed.
#include "board.h"
#include "urtos.h"

namespace {

constexpr uint32_t kTickReload = 24999;       // a tick every 25,000 processor clocks
constexpr uint32_t kStopTick = 48;            // four hyperperiods of 12 ticks

uint32_t loopsPerTenthTick = 0;               // calibrated before the scheduler starts

// Burn `tenths` x 0.1 tick of processor time. Never inlined, so that every caller runs
// exactly the loop that calibrate() measured.
__attribute__((noinline)) void work(uint32_t tenths)
{
    for (volatile uint32_t i = 0; i < tenths * loopsPerTenthTick; i = i + 1) {
    }
}

struct Periodic {
    const char* name;
    char letter;
    uint8_t priority;
    uint32_t period;      // ticks; the deadline equals the period
    uint32_t cost;        // tenths of a tick of work per job
    uint32_t jobs = 0, misses = 0, worst = 0;
};

// TASK SET (the forensic lab's app_v2.cc differs only in these three priority numbers)
Periodic sensor{"sensor", 'A', 3, 4, 8};
Periodic control{"control", 'B', 2, 6, 18};
Periodic filter{"filter", 'C', 1, 12, 28};

constexpr uint32_t clocksPerTick = kTickReload + 1;   // processor clocks per tick

template <Periodic& P>
void periodicTask()
{
    uint32_t release = 0;
    for (;;) {
        work(P.cost);
        const uint32_t response = os::now() - os::current().releaseTime;
        P.jobs = P.jobs + 1;
        if (response > P.worst) { P.worst = response; }
        if (response > P.period * clocksPerTick) { P.misses = P.misses + 1; }
        release += P.period;
        os::sleepUntil(release);
    }
}

uint32_t sensorStack[256], controlStack[256], filterStack[256];

void printTenths(uint32_t clocks)             // clocks as ticks with one decimal
{
    const uint32_t tenths = (clocks * 10 + clocksPerTick / 2) / clocksPerTick;
    board::printDec(tenths / 10); board::putChar('.'); board::printDec(tenths % 10);
}

void report()
{
    board::print("timeline, one character per quarter tick (A sensor, B control, C filter, . idle):\n");
    for (uint32_t row = 0; row < 2; ++row) {
        board::print(row == 0 ? "ticks  0-11 |" : "ticks 12-23 |");
        for (uint32_t col = 0; col < 48; ++col) {
            const uint32_t t = (row * 48 + col) * clocksPerTick / 4 + clocksPerTick / 8;
            char who = '?';
            for (uint32_t i = 0; i < os::switchCount && os::switchLog[i].time <= t; ++i) {
                who = os::switchLog[i].to;
            }
            board::putChar(who);
            if (col % 4 == 3) { board::putChar('|'); }
        }
        board::print("\n");
    }
    board::print("task     prio period cost  jobs  worst response  deadline misses\n");
    Periodic* const all[] = {&sensor, &control, &filter};
    for (Periodic* p : all) {
        board::print(p->name); board::print(p->letter == 'A' ? "     " : "    ");
        board::printDec(p->priority); board::print("    ");
        board::printDec(p->period); board::print(p->period < 10 ? "     " : "    ");
        board::printDec(p->cost / 10); board::putChar('.'); board::printDec(p->cost % 10);
        board::print("   "); board::printDec(p->jobs); board::print("    ");
        printTenths(p->worst); board::print(" ticks       "); board::printDec(p->misses);
        board::print("\n");
    }
    board::print("context switches logged: "); board::printDec(os::switchCount); board::print("\n");
    const bool ok = sensor.misses == 0 && control.misses == 0 && filter.misses == 0;
    board::print(ok ? "all deadlines met\n" : "DEADLINE MISSED\n");
    board::exitEmulator(true);
}

void calibrate()
{
    // Run a fixed busy loop while SysTick counts processor clocks (24-bit, counting down).
    board::reg(board::kSystRvr) = 0x00FFFFFF;
    board::reg(board::kSystCvr) = 0;
    board::reg(board::kSystCsr) = 0x5;                       // processor clock, enable
    loopsPerTenthTick = 2000;                                // provisional: work(10) = 20000 loops
    const uint32_t s0 = board::reg(board::kSystCvr);
    work(10);
    const uint32_t cpu = (s0 - board::reg(board::kSystCvr)) & 0x00FFFFFFu;   // 24-bit counter
    board::reg(board::kSystCsr) = 0;
    loopsPerTenthTick = 20000u * ((kTickReload + 1) / 10) / cpu;
    board::print("calibration: 20000 loops took "); board::printDec(cpu);
    board::print(" processor clocks; loops per 0.1 tick = "); board::printDec(loopsPerTenthTick);
    board::print("\n");
}

}  // namespace

int main()
{
    board::uartInit();
    board::print("F3-39: uRTOS, three periodic tasks, fixed priorities\n");
    calibrate();
    os::createTask(periodicTask<sensor>, sensor.priority, sensorStack, 256, sensor.name, 'A');
    os::createTask(periodicTask<control>, control.priority, controlStack, 256, control.name, 'B');
    os::createTask(periodicTask<filter>, filter.priority, filterStack, 256, filter.name, 'C');
    os::start(kTickReload, kStopTick, report);
}
