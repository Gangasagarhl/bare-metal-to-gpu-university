// sched_sim.cpp - the university's scheduling simulator: one CPU, jobs with arrival times,
// CPU bursts and priorities, and several scheduling policies. Time is counted in ticks.
#include <cstdio>
#include <deque>
#include <iostream>
#include <string>
#include <vector>

struct Job {
    char name;
    int arrival, burst, priority;  // priority: 1 is the most urgent
    int left = 0, first_run = -1, finish = -1, waited = 0;
};

// Picks the index of the job to run at time t, or -1 for "CPU idle".
struct Scheduler {
    std::string policy;
    int quantum = 1;  // RR: ticks before a job must give the CPU back
    int aging = 0;    // PRIO: every 'aging' ticks of waiting, priority improves by one
    std::deque<int> queue;
    int running = -1, used = 0;

    int effective(const Job& j) const
    {
        return aging > 0 ? j.priority - j.waited / aging : j.priority;
    }
    int pick(std::vector<Job>& jobs, int t)
    {
        for (int i = 0; i < static_cast<int>(jobs.size()); ++i) {
            if (jobs[i].arrival == t) { queue.push_back(i); }  // arrivals join the back
        }
        if (running >= 0 && jobs[running].left == 0) { running = -1; }
        const bool preemptive = policy != "FIFO" && policy != "SJF";
        if (policy == "RR" && running >= 0 && used == quantum) {
            queue.push_back(running);  // time slice over: back of the queue
            running = -1;
        }
        if (running >= 0 && !preemptive) { return running; }
        if (running >= 0 && policy == "RR") { return running; }
        if (running >= 0) {  // STCF, PRIO: put it back and choose again
            queue.push_back(running);
            running = -1;
        }
        if (queue.empty()) { return -1; }
        auto best = queue.begin();
        for (auto it = queue.begin(); it != queue.end(); ++it) {
            const Job& a = jobs[*it];
            const Job& b = jobs[*best];
            bool better = false;
            if (policy == "SJF") { better = a.burst < b.burst; }
            if (policy == "STCF") { better = a.left < b.left; }
            if (policy == "PRIO") { better = effective(a) < effective(b); }
            if (better) { best = it; }  // ties: earlier in the queue wins
        }
        if (policy == "FIFO" || policy == "RR") { best = queue.begin(); }
        const int chosen = *best;
        queue.erase(best);
        if (chosen != running) { used = 0; }
        running = chosen;
        return chosen;
    }
};

int main()
{
    int njobs = 0, horizon = 0;
    std::cin >> njobs >> horizon;
    std::vector<Job> input;
    for (int i = 0; i < njobs; ++i) {
        Job j{};
        std::cin >> j.name >> j.arrival >> j.burst >> j.priority;
        j.left = j.burst;
        input.push_back(j);
    }
    std::string policy;
    while (std::cin >> policy) {
        Scheduler s;
        s.policy = policy;
        if (policy == "RR") { std::cin >> s.quantum; }
        if (policy == "PRIO") { std::cin >> s.aging; }
        std::vector<Job> jobs = input;
        std::string timeline;
        for (int t = 0; t < horizon; ++t) {
            const int i = s.pick(jobs, t);
            for (int k : s.queue) { ++jobs[k].waited; }
            if (i < 0) {
                timeline += '.';
                continue;
            }
            Job& j = jobs[i];
            if (j.first_run < 0) { j.first_run = t; }
            timeline += j.name;
            --j.left;
            ++s.used;
            if (j.left == 0) { j.finish = t + 1; }
        }
        std::printf("%s", policy.c_str());
        if (policy == "RR") { std::printf(" (time slice %d)", s.quantum); }
        if (policy == "PRIO") {
            std::printf(s.aging > 0 ? " (aging every %d ticks)" : " (no aging)", s.aging);
        }
        std::printf("\n  timeline: %s\n", timeline.c_str());
        double turn = 0, resp = 0;
        int finished = 0;
        for (const Job& j : jobs) {
            if (j.finish < 0) {
                std::printf("  %c: NOT FINISHED after %d ticks (ran %d of %d)\n", j.name, horizon,
                            j.burst - j.left, j.burst);
                continue;
            }
            std::printf(
                "  %c: arrival %2d  first run %2d  finish %2d  turnaround %2d  response %2d\n",
                j.name, j.arrival, j.first_run, j.finish, j.finish - j.arrival,
                j.first_run - j.arrival);
            turn += j.finish - j.arrival;
            resp += j.first_run - j.arrival;
            ++finished;
        }
        if (finished > 0) {
            std::printf("  average turnaround %.2f  average response %.2f  (%d jobs finished)\n\n",
                        turn / finished, resp / finished, finished);
        }
    }
    return 0;
}
