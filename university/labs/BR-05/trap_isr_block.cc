// trap_isr_block.cc - BR-05 Listing 8: trap 1, a blocking call inside an interrupt handler.
// The control step runs in the timer interrupt (as in Listing 6) and logs every command into
// a small ring buffer that main() empties to the UART. When the buffer is full, logBlocking()
// WAITS for main() to make room. main() can never run while the handler waits: the firmware
// stops. BR05_FIX=1 builds the fixed version: logTry() never waits; it counts what it drops.
#include "bm_time.h"
#include "board.h"
#include "loop_core.h"
#include "mcu_common.h"

#ifndef BR05_FIX
#define BR05_FIX 0
#endif

namespace {

constexpr uint32_t kRun = 60;                            // periods in this demonstration
constexpr uint32_t kLogSize = 8;
volatile uint32_t logBuf[kLogSize];
volatile uint32_t head = 0;                              // written only by the handler
volatile uint32_t tail = 0;                              // written only by main()
volatile uint32_t dropped = 0;
volatile bool finished = false;
br05::Loop loop;

void logBlocking(uint32_t value)                         // WRONG in a handler: may wait
{
    if (head - tail == kLogSize) {
        board::print("  [handler] log full at period ");
        board::printDec(bm::ticks);
        board::print(": waiting for main() to make room ...\n");
    }
    while (head - tail == kLogSize) {
        // wait for main() to free a slot. main() is the code this handler interrupted.
    }
    logBuf[head % kLogSize] = value;
    head = head + 1;
}

bool logTry(uint32_t value)                              // right: never waits
{
    if (head - tail == kLogSize) {
        dropped = dropped + 1;
        return false;
    }
    logBuf[head % kLogSize] = value;
    head = head + 1;
    return true;
}

}  // namespace

extern "C" void SysTick_Handler()
{
    const uint32_t k = bm::ticks + 1;
    bm::ticks = k;
    if (k > kRun) {
        finished = true;
        return;
    }
    const int32_t u = loop.step();
    board::setLeds(static_cast<uint32_t>(u) >> 7);
    if (BR05_FIX) {
        logTry(static_cast<uint32_t>(u));
    } else {
        logBlocking(static_cast<uint32_t>(u));
    }
}

int main()
{
    board::uartInit();
    board::print(BR05_FIX ? "BR-05 trap 1, fixed: the handler never waits\n"
                          : "BR-05 trap 1: the handler waits for a full log buffer\n");
    mcu::calibrate();
    bm::startTicks();
    uint32_t printed = 0;
    while (!finished) {
        while (tail != head) {                           // empty the log to the UART
            const uint32_t v = logBuf[tail % kLogSize];
            tail = tail + 1;
            printed = printed + 1;
            if (printed % 10 == 0) {
                board::print("  main: logged command ");
                board::printDec(v);
                board::print(" (entry ");
                board::printDec(printed);
                board::print(")\n");
            }
        }
        mcu::work(100);                                  // a slow job: 10 periods
    }
    board::reg(board::kSystCsr) = 0;
    board::print("finished ");
    board::printDec(kRun);
    board::print(" periods; entries logged ");
    board::printDec(printed);
    board::print(", dropped ");
    board::printDec(dropped);
    board::print("\n");
    board::exitEmulator(true);
}
