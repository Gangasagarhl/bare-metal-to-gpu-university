// vehicle_fw.cc - F10-27 Listing 4: the microcontroller backend of the teaching HAL, on
// uRTOS (the OS305 teaching RTOS, files copied unchanged from labs/F3-39) in QEMU's
// mps2-an385 board. Each periodic task becomes an RTOS thread with its own stack. Every
// stack is "painted" with a pattern before the thread starts, so that the report can count
// how much of it was ever written (the stack's high-water mark).
#include "board.h"
#include "hal.h"
#include "sim_airframe.h"
#include "urtos.h"
#include "vehicle.h"

extern "C" void __cxa_pure_virtual()       // needed by abstract classes without a C++ runtime
{
    board::print("pure virtual call\n");
    board::exitEmulator(false);
}

namespace {

constexpr uint32_t kTickReload = 24999;     // one tick = 25,000 processor clocks
constexpr uint32_t kStopTick = 40;
constexpr uint32_t kStackWords = 256;       // 1 KiB per thread
constexpr uint32_t kGuardWords = 128;       // 512 bytes of guard below each stack
constexpr uint32_t kPaint = 0x55AA55AAu;

sim::Airframe airframe;

class FwConsole final : public hal::Console {
public:
    void write(const char* text) override { board::print(text); }
};

class FwGyro final : public hal::Gyro {
public:
    int32_t readRollRate() override { return airframe.rollRate; }
};

class FwMotor final : public hal::MotorOut {
public:
    void write(int32_t command) override { airframe.apply(command); }
};

class FwLed final : public hal::Led {
public:
    void set(bool on) override { board::setLeds(on ? 1u : 0u); }
};

// Guard below, stack above, in one struct so that the linker cannot reorder them.
struct alignas(8) StackArea {
    uint32_t guard[kGuardWords];
    uint32_t stack[kStackWords];
};

struct Slot {
    hal::Scheduler::Task task;
    uint32_t period;
    uint8_t priority;
    const char* name;
    uint32_t runs;
};

constexpr uint32_t kMaxSlots = 4;
Slot slots[kMaxSlots];
StackArea areas[kMaxSlots];
uint32_t slotCount = 0;

template <uint32_t I>
void periodicThread()                       // one RTOS thread per registered task
{
    uint32_t release = 0;
    for (;;) {
        slots[I].task();
        slots[I].runs = slots[I].runs + 1;
        release += slots[I].period;
        os::sleepUntil(release);
    }
}

constexpr os::TaskFunction kThreads[kMaxSlots] = {periodicThread<0>, periodicThread<1>,
                                                  periodicThread<2>, periodicThread<3>};

void report();

class FwScheduler final : public hal::Scheduler {
public:
    void addPeriodic(Task task, uint32_t periodTicks, uint8_t priority,
                     const char* name) override
    {
        if (slotCount == kMaxSlots) {
            board::print("too many tasks\n");
            board::exitEmulator(false);
        }
        slots[slotCount] = Slot{task, periodTicks, priority, name, 0};
        slotCount = slotCount + 1;
    }
    uint32_t ticks() override { return os::tick(); }
    void run(uint32_t stopTick) override
    {
        for (uint32_t i = 0; i < slotCount; ++i) {
            for (uint32_t w = 0; w < kGuardWords; ++w) { areas[i].guard[w] = kPaint; }
            for (uint32_t w = 0; w < kStackWords; ++w) { areas[i].stack[w] = kPaint; }
            os::createTask(kThreads[i], slots[i].priority, areas[i].stack, kStackWords,
                           slots[i].name, static_cast<char>('A' + i));
        }
        os::start(kTickReload, stopTick, report);
    }
};

FwConsole console;
FwGyro gyro;
FwMotor motor;
FwLed led;
FwScheduler scheduler;

void report()
{
    bool ok = true;
    for (uint32_t i = 0; i < slotCount; ++i) {
        uint32_t untouched = 0;                 // painted words still intact, from the bottom
        while (untouched < kStackWords && areas[i].stack[untouched] == kPaint) {
            ++untouched;
        }
        uint32_t damaged = 0;                   // guard words that were written
        for (uint32_t w = 0; w < kGuardWords; ++w) {
            if (areas[i].guard[w] != kPaint) { ++damaged; }
        }
        board::print("task ");
        board::print(slots[i].name);
        board::print(": runs ");
        board::printDec(slots[i].runs);
        board::print(", stack high-water ");
        board::printDec((kStackWords - untouched) * 4);
        board::print(" of ");
        board::printDec(kStackWords * 4);
        board::print(" bytes, guard words written ");
        board::printDec(damaged);
        board::print("\n");
        if (damaged != 0 || untouched == 0) {
            ok = false;
        }
    }
    board::print(ok ? "stack check: PASS\n" : "stack check: FAIL (a thread overflowed its stack)\n");
    board::exitEmulator(ok);
}

}  // namespace

int main()
{
    board::uartInit();
    board::print("F10-27 uRTOS backend on the emulated Cortex-M3 (LOG_LINE_BYTES=");
    board::printDec(LOG_LINE_BYTES);
    board::print(")\n");
    static const hal::Hal halObject{console, gyro, motor, led, scheduler};
    vehicle::setup(halObject);
    scheduler.run(kStopTick);          // starts the RTOS; does not come back
    return 0;
}
