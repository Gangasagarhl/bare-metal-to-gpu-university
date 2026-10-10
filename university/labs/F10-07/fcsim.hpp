// F10-07: a tiny fixed-priority preemptive scheduler simulator in 1-microsecond steps,
// used by sched_sim.cpp (Listing 2) and jitter_log.cpp (forensic evidence).
// Task periods and run times are the university's exercise values, not measurements
// of any real flight controller.
#pragma once
#include <string>
#include <vector>

struct Task
{
    std::string name;
    int period;      // us; the task is released at 0, period, 2*period, ...
    int cost;        // us of CPU time per release
    int priority;    // smaller number = more urgent
    int lockedPart;  // us at the start of each job that cannot be preempted (0 = none)
    // results
    int remaining = 0;
    int released = 0;     // release time of the current job
    int jobs = 0;
    int misses = 0;       // jobs not finished before the next release
    int worstResponse = 0;
    int ran = 0;          // us already run in the current job
};

struct StartRecord
{
    int release;   // when the job was released
    int start;     // when it first got the CPU
};

// Runs the task set for `duration` us. `trace` (if not null) receives the start
// records of task 0, the task watched by the forensic log.
inline void simulate(std::vector<Task>& tasks, int duration, std::vector<StartRecord>* trace)
{
    int locked = -1;          // index of a task inside its non-preemptible part
    int lastStarted = -1;
    for (int t = 0; t < duration; ++t) {
        for (auto& k : tasks) {
            if (t % k.period == 0) {
                if (k.remaining > 0) {
                    ++k.misses;               // previous job still running: deadline missed
                }
                k.remaining = k.cost;
                k.released = t;
                k.ran = 0;
                ++k.jobs;
            }
        }
        int run = locked;
        if (run < 0) {
            for (int i = 0; i < static_cast<int>(tasks.size()); ++i) {
                if (tasks[i].remaining > 0 &&
                    (run < 0 || tasks[i].priority < tasks[run].priority)) {
                    run = i;
                }
            }
        }
        if (run < 0) {
            continue;                         // idle
        }
        Task& k = tasks[run];
        if (k.ran == 0 && trace != nullptr && run == 0 && lastStarted != k.jobs) {
            trace->push_back({k.released, t});
            lastStarted = k.jobs;
        }
        ++k.ran;
        --k.remaining;
        locked = (k.ran < k.lockedPart && k.remaining > 0) ? run : -1;
        if (k.remaining == 0) {
            const int response = t + 1 - k.released;
            if (response > k.worstResponse) {
                k.worstResponse = response;
            }
        }
    }
}
