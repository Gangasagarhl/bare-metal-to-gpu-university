// F10-07 Listing 2: the flight controller's tasks under a fixed-priority preemptive
// scheduler (OS305), simulated for one second. Two logger designs are compared:
// A writes a block to the SD card in one non-preemptible piece, B lets the rate
// loop preempt it at any time (for example by handing the write to DMA and a queue).
#include "fcsim.hpp"

#include <cstdio>
#include <vector>

std::vector<Task> taskSet(int loggerLocked)
{
    // name, period us, cost us, priority, non-preemptible us
    return {{"rate loop", 1000, 250, 0, 0},
            {"sensors", 1000, 150, 1, 0},
            {"estimator", 4000, 600, 2, 0},
            {"position", 20000, 1500, 3, 0},
            {"logger", 20000, 2500, 4, loggerLocked}};
}

void report(const char* title, std::vector<Task> tasks)
{
    simulate(tasks, 1000000, nullptr);
    double load = 0.0;
    std::printf("%s\n", title);
    std::printf("  %-10s %7s %6s %5s %6s %8s\n", "task", "period", "cost", "jobs", "misses",
                "worst R");
    for (const auto& k : tasks) {
        load += static_cast<double>(k.cost) / k.period;
        std::printf("  %-10s %7d %6d %5d %6d %8d\n", k.name.c_str(), k.period, k.cost, k.jobs,
                    k.misses, k.worstResponse);
    }
    std::printf("  CPU load %.1f %%\n\n", 100.0 * load);
}

int main()
{
    report("A: logger writes 2500 us without preemption", taskSet(2500));
    report("B: logger fully preemptible", taskSet(0));
    return 0;
}
