// vehicle_host.cpp - F10-27 Listing 3: the host backend of the teaching HAL. Time is
// simulated: run() steps a tick counter and, at each tick, runs the due tasks in priority
// order (most urgent first), as a preemptive RTOS would order them. This plays the role
// that a software-in-the-loop (SITL) backend plays for a real flight stack.
#include <algorithm>
#include <cstdio>
#include <vector>

#include "hal.h"
#include "sim_airframe.h"
#include "vehicle.h"

namespace {

sim::Airframe airframe;

class HostConsole final : public hal::Console {
public:
    void write(const char* text) override { std::fputs(text, stdout); }
};

class HostGyro final : public hal::Gyro {
public:
    int32_t readRollRate() override { return airframe.rollRate; }
};

class HostMotor final : public hal::MotorOut {
public:
    void write(int32_t command) override { airframe.apply(command); }
};

class HostLed final : public hal::Led {
public:
    int toggles = 0;
    bool state = false;
    void set(bool on) override
    {
        if (on != state) { ++toggles; }
        state = on;
    }
};

class HostScheduler final : public hal::Scheduler {
public:
    struct Entry {
        Task task;
        uint32_t period;
        uint8_t priority;
        const char* name;
        uint32_t runs;
    };
    std::vector<Entry> entries;
    uint32_t now = 0;

    void addPeriodic(Task task, uint32_t periodTicks, uint8_t priority,
                     const char* name) override
    {
        entries.push_back(Entry{task, periodTicks, priority, name, 0});
        std::stable_sort(entries.begin(), entries.end(),
                         [](const Entry& a, const Entry& b) { return a.priority > b.priority; });
    }
    uint32_t ticks() override { return now; }
    void run(uint32_t stopTick) override
    {
        for (now = 0; now < stopTick; ++now) {
            for (Entry& e : entries) {
                if (now % e.period == 0) {
                    e.task();
                    ++e.runs;
                }
            }
        }
    }
};

}  // namespace

int main()
{
    HostConsole console;
    HostGyro gyro;
    HostMotor motor;
    HostLed led;
    HostScheduler scheduler;
    const hal::Hal halObject{console, gyro, motor, led, scheduler};

    std::puts("F10-27 host backend (simulated time)");
    vehicle::setup(halObject);
    scheduler.run(40);
    for (const auto& e : scheduler.entries) {
        std::printf("task %-5s priority %u period %2u ticks: %u runs\n", e.name,
                    static_cast<unsigned>(e.priority), static_cast<unsigned>(e.period),
                    static_cast<unsigned>(e.runs));
    }
    std::printf("LED toggles: %d\n", led.toggles);
    return 0;
}
