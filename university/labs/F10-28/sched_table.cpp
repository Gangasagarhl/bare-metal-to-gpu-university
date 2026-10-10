// sched_table.cpp - F10-28 Listing 1: a 400 Hz main loop with a task table, run for 10
// simulated seconds. Design A is a healthy table. Design B adds one new 400 Hz task with a
// large budget near the top of the table, as a developer might when adding a feature.
#include <vector>

#include "aploop.hpp"

std::vector<aploop::Task> baseTable()
{
    // name, rate (Hz), budget (us), typical cost (us): exercise values
    return {
        {"rc_input", 400, 100, 40},
        {"gps", 50, 200, 120},
        {"navigation", 100, 300, 180},
        {"compass", 100, 100, 60},
        {"baro", 50, 150, 90},
        {"telemetry", 50, 500, 300},
        {"logging", 100, 400, 250},
        {"battery", 10, 120, 70},
        {"arming", 1, 50, 30},
    };
}

int main()
{
    constexpr int loopHz = 400;
    constexpr int fastLoopUs = 1100;
    constexpr int seconds = 10;

    std::printf("Design A: the healthy table (loop %d Hz, fast loop %d us)\n", loopHz,
                fastLoopUs);
    auto a = baseTable();
    const auto ra = aploop::simulate(a, loopHz, fastLoopUs, seconds);
    aploop::report(a, ra, loopHz, seconds);

    std::printf("\nDesign B: a new 400 Hz 'object_avoid' task (budget 1000 us) inserted "
                "after rc_input\n");
    auto b = baseTable();
    b.insert(b.begin() + 1, aploop::Task{"object_avoid", 400, 1000, 800});
    const auto rb = aploop::simulate(b, loopHz, fastLoopUs, seconds);
    aploop::report(b, rb, loopHz, seconds);
    return 0;
}
