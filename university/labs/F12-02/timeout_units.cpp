// timeout_units.cpp - SE301 F12-02 forensic evidence generator.
// The design doc said only: "bool rx_wait(int timeout) waits until a byte is available
// or the timeout expires." The driver team read the number as milliseconds; the console
// team, which waits for a modem's answer during boot, passed 5000 meaning microseconds.
// A simulated clock (in microseconds) replays three boots; a boot watchdog resets the
// board if boot has not finished after 2 s of simulated time.
#include <cstdio>
#include <iostream>
#include <string>

struct Sim
{
    long nowUs = 0;
    long byteAtUs = -1;                                   // -1: no byte ever arrives
    bool byteReady() const { return byteAtUs >= 0 && nowUs >= byteAtUs; }
};

constexpr long kWatchdogUs = 2'000'000;

// Driver team's implementation: "timeout" in milliseconds, polled every 100 us.
// Returns 1 (byte), 0 (timed out) or -1 (the watchdog reset the board while waiting).
static int rx_wait(Sim& sim, int timeout)
{
    const long deadline = sim.nowUs + timeout * 1000L;
    while (!sim.byteReady()) {
        if (sim.nowUs >= deadline) {
            return 0;
        }
        if (sim.nowUs >= kWatchdogUs) {
            return -1;
        }
        sim.nowUs += 100;
    }
    return 1;
}

int main()
{
    std::string arg;
    while (std::cin >> arg) {
        Sim sim;
        sim.byteAtUs = (arg == "never") ? -1 : std::stol(arg);
        std::printf("boot with modem answer at: %s%s\n", arg.c_str(), arg == "never" ? "" : " us");
        std::printf("  t=%8ld us  console: rx_wait(5000), expecting to give up after 5 ms\n",
                    sim.nowUs);
        const int r = rx_wait(sim, 5000);
        if (r == -1) {
            std::printf("  t=%8ld us  WATCHDOG: boot not finished after 2 s, board reset\n",
                        sim.nowUs);
        } else {
            std::printf("  t=%8ld us  rx_wait returned %s\n", sim.nowUs, r ? "true" : "false");
            const bool consoleExpected = sim.byteAtUs >= 0 && sim.byteAtUs <= 5000;
            if (static_cast<bool>(r) != consoleExpected) {
                std::printf("  t=%8ld us  console expected %s: modem state now wrong\n",
                            sim.nowUs, consoleExpected ? "true" : "false");
            }
        }
    }
    return 0;
}
