// rr_model.cpp - a host model of one CPU: cooperative scheduling versus timer-driven
// round robin, with one thread that never gives the CPU back.
// Not kernel code: plain C++ that counts "time slices" so the policies can be compared.
#include <cstdio>
#include <deque>
#include <string>
#include <vector>

struct Job {
    std::string name;
    int work_left;     // slices of work it still needs; -1 = runs forever (the spinner)
    int yields_every;  // gives the CPU back after this many slices if not preempted; 0 = never
    int finished_at = -1;
};

// Simulates 'total' slices. With preempt = true the timer takes the CPU away after
// 'quantum' slices; otherwise a job keeps the CPU until it yields or finishes.
void simulate(std::vector<Job> jobs, bool preempt, int quantum, int total)
{
    std::deque<int> ready;
    for (int i = 0; i < static_cast<int>(jobs.size()); ++i) {
        ready.push_back(i);
    }
    int t = 0;
    while (t < total && !ready.empty()) {
        int cur = ready.front();
        ready.pop_front();
        Job& j = jobs[static_cast<std::size_t>(cur)];
        int ran = 0;
        while (t < total) {
            ++t;
            ++ran;
            if (j.work_left > 0 && --j.work_left == 0) {
                j.finished_at = t;
                break;
            }
            if (preempt && ran == quantum) {
                break;   // timer interrupt: back of the queue
            }
            if (!preempt && j.yields_every > 0 && ran == j.yields_every) {
                break;   // the job yields voluntarily
            }
        }
        if (j.finished_at < 0) {
            ready.push_back(cur);
        }
    }
    std::printf("%s, %d slices:\n", preempt ? "preemptive round robin (quantum 2)" : "cooperative", total);
    for (const Job& j : jobs) {
        if (j.finished_at >= 0) {
            std::printf("  %-8s finished at slice %d\n", j.name.c_str(), j.finished_at);
        } else {
            std::printf("  %-8s not finished\n", j.name.c_str());
        }
    }
}

int main()
{
    std::vector<Job> jobs = {
        {"editor", 4, 1},     // short job that yields after every slice
        {"spinner", -1, 0},   // an infinite loop that never yields
        {"backup", 6, 2},
    };
    simulate(jobs, false, 2, 40);
    simulate(jobs, true, 2, 40);
    return 0;
}
