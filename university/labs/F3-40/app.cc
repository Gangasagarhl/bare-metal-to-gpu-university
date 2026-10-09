// app.cc - F3-40 Listing 3: a sensor task with a measured deadline on uRTOS v2.
//   sensor  (priority 3): every 5 ticks, from tick 1: read the I2C temperature sensor while
//                         holding the bus mutex, filter it (0.3 tick), send it to the queue.
//                         Deadline: 2 ticks after each release.
//   compute (priority 2): every 10 ticks, from tick 1: 3 ticks of number crunching, no bus.
//   logger  (priority 1): every 10 ticks, from tick 0: print queued readings, then write a
//                         log record to an (imagined) slow I2C memory: 1.5 ticks holding the bus.
// OS305_INHERITANCE (0 or 1, set by the build) switches priority inheritance off or on.
#include "board.h"
#include "i2c_bitbang.h"
#include "urtos.h"

#ifndef OS305_INHERITANCE
#error "build with -DOS305_INHERITANCE=0 or -DOS305_INHERITANCE=1"
#endif

namespace {

constexpr uint32_t kTickReload = 24999;            // 25,000 processor clocks per tick
constexpr uint32_t kClocksPerTick = kTickReload + 1;
constexpr uint32_t kStopTick = 40;
constexpr uint32_t kDeadlineTicks = 2;

uint32_t loopsPerTenthTick = 0;

// Burn `tenths` x 0.1 tick of processor time. Never inlined, so that every caller runs
// exactly the loop that calibrate() measured.
__attribute__((noinline)) void work(uint32_t tenths)
{
    for (volatile uint32_t i = 0; i < tenths * loopsPerTenthTick; i = i + 1) {
    }
}

struct SbconPins {                                  // as in F3-37
    static constexpr uintptr_t kBase = 0x4002A000;
    void scl(bool high) { board::reg(kBase + (high ? 0x0 : 0x4)) = 1u; }
    void sda(bool high) { board::reg(kBase + (high ? 0x0 : 0x4)) = 2u; }
    bool readSda() { return (board::reg(kBase) & 2u) != 0; }
    void wait() {}
};
SbconPins pins;
I2cMaster<SbconPins> i2c(pins);

os::Mutex busMutex(OS305_INHERITANCE != 0);        // protects the I2C bus
os::Queue readings;                                 // sensor -> logger

uint32_t responses[16];                             // sensor response times, clocks
uint32_t sensorJobs = 0, sensorMisses = 0;

void sensorTask()
{
    uint32_t release = 1;
    os::sleepUntil(release);
    for (;;) {
        busMutex.lock();
        const uint8_t pointer = 0x00;
        uint8_t raw[2] = {0, 0};
        const bool ok = i2c.transfer(0x48, &pointer, 1, raw, 2);
        busMutex.unlock();
        work(3);                                    // filtering: 0.3 tick of processing
        const int32_t centi = (static_cast<int16_t>((raw[0] << 8) | raw[1]) * 100) / 256;
        readings.send(ok ? static_cast<uint32_t>(centi) : 0xFFFFFFFFu);
        const uint32_t r = os::now() - os::current().releaseTime;
        if (sensorJobs < 16) { responses[sensorJobs] = r; }
        sensorJobs = sensorJobs + 1;
        if (r > kDeadlineTicks * kClocksPerTick) { sensorMisses = sensorMisses + 1; }
        release += 5;
        os::sleepUntil(release);
    }
}

void computeTask()
{
    uint32_t release = 1;
    os::sleepUntil(release);
    for (;;) {
        work(30);
        release += 10;
        os::sleepUntil(release);
    }
}

void loggerTask()
{
    uint32_t release = 0;
    for (;;) {
        uint32_t v = 0;
        while (readings.tryReceive(v)) {
            board::print("  logger: reading ");
            board::printDec(v / 100); board::putChar('.'); board::printDec((v / 10) % 10);
            board::printDec(v % 10); board::print(" C\n");
        }
        busMutex.lock();
        work(15);                                   // the slow log write on the shared bus
        busMutex.unlock();
        release += 10;
        os::sleepUntil(release);
    }
}

uint32_t sensorStack[256], computeStack[256], loggerStack[256];

void printTicks(uint32_t clocks)                    // clocks as ticks, two decimals
{
    const uint32_t hundredths = (clocks * 4) / 1000;   // clocks / 250 = 1/100 tick
    board::printDec(hundredths / 100); board::putChar('.');
    board::printDec((hundredths / 10) % 10); board::printDec(hundredths % 10);
}

void report()
{
    board::print("timeline, one character per quarter tick (S sensor, C compute, L logger, . idle):\n");
    for (uint32_t row = 0; row < 2; ++row) {
        board::print(row == 0 ? "ticks  0-9  |" : "ticks 10-19 |");
        for (uint32_t col = 0; col < 40; ++col) {
            const uint32_t t = (row * 40 + col) * kClocksPerTick / 4 + kClocksPerTick / 8;
            char who = '?';
            for (uint32_t i = 0; i < os::switchCount && os::switchLog[i].time <= t; ++i) {
                who = os::switchLog[i].to;
            }
            board::putChar(who);
            if (col % 4 == 3) { board::putChar('|'); }
        }
        board::print("\n");
    }
    board::print("sensor response times (ticks):");
    for (uint32_t i = 0; i < sensorJobs && i < 16; ++i) {
        board::putChar(' '); printTicks(responses[i]);
    }
    board::print("\nsensor jobs "); board::printDec(sensorJobs);
    board::print(", deadline "); board::printDec(kDeadlineTicks);
    board::print(" ticks, misses "); board::printDec(sensorMisses);
    board::print(OS305_INHERITANCE ? "  [priority inheritance ON]\n" : "  [priority inheritance OFF]\n");
    board::exitEmulator(true);
}

void calibrate()
{
    board::reg(board::kSystRvr) = 0x00FFFFFF;
    board::reg(board::kSystCvr) = 0;
    board::reg(board::kSystCsr) = 0x5;
    loopsPerTenthTick = 2000;                                // provisional: work(10) = 20000 loops
    const uint32_t s0 = board::reg(board::kSystCvr);
    work(10);
    const uint32_t cpu = (s0 - board::reg(board::kSystCvr)) & 0x00FFFFFFu;
    board::reg(board::kSystCsr) = 0;
    loopsPerTenthTick = 20000u * (kClocksPerTick / 10) / cpu;
}

}  // namespace

int main()
{
    board::uartInit();
    board::print("F3-40: sensor task with a deadline, uRTOS v2 (mutex + queue)\n");
    calibrate();
    os::createTask(sensorTask, 3, sensorStack, 256, "sensor", 'S');
    os::createTask(computeTask, 2, computeStack, 256, "compute", 'C');
    os::createTask(loggerTask, 1, loggerStack, 256, "logger", 'L');
    os::start(kTickReload, kStopTick, report);
}
