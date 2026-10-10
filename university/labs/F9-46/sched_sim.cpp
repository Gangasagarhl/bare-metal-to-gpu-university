// sched_sim.cpp - F9-46 Listing 1: simulate rate-monotonic (RM) and earliest-deadline-first
// (EDF) scheduling of periodic tasks on one processor, in whole time units.
// Deadlines equal periods. A job that misses its deadline keeps running until it is done
// (the behaviour of most real systems unless the program aborts late jobs).
#include <cstdio>
#include <string>
#include <vector>

struct Task {
    char name;
    int period;   // T: a job is released every T units; its deadline is the next release
    int cost;     // C: units of processor time per job (worst case)
};

struct Job {
    int task;
    int release;
    int deadline;  // absolute
    int left;      // units of work still to do
};

enum class Policy { RM, EDF };

// Is job a more urgent than job b under the policy? Ties go to the lower task index.
bool moreUrgent(const Job& a, const Job& b, const std::vector<Task>& ts, Policy p)
{
    if (p == Policy::RM) {
        if (ts[a.task].period != ts[b.task].period) { return ts[a.task].period < ts[b.task].period; }
    } else {
        if (a.deadline != b.deadline) { return a.deadline < b.deadline; }
    }
    return a.task < b.task;
}

void simulate(const char* title, const std::vector<Task>& ts, Policy p, int horizon)
{
    std::vector<Job> ready;
    std::string line;
    std::vector<int> jobs(ts.size(), 0), misses(ts.size(), 0), worst(ts.size(), 0);
    for (int t = 0; t < horizon; ++t) {
        for (std::size_t i = 0; i < ts.size(); ++i) {
            if (t % ts[i].period == 0) {
                ready.push_back(Job{static_cast<int>(i), t, t + ts[i].period, ts[i].cost});
            }
        }
        int pick = -1;
        for (std::size_t k = 0; k < ready.size(); ++k) {
            if (pick < 0 || moreUrgent(ready[k], ready[static_cast<std::size_t>(pick)], ts, p)) {
                pick = static_cast<int>(k);
            }
        }
        if (pick < 0) {
            line += '.';
            continue;
        }
        Job& j = ready[static_cast<std::size_t>(pick)];
        line += ts[static_cast<std::size_t>(j.task)].name;
        j.left -= 1;
        if (j.left == 0) {                        // job completes at the end of unit t
            const std::size_t i = static_cast<std::size_t>(j.task);
            const int response = t + 1 - j.release;
            jobs[i] += 1;
            if (response > worst[i]) { worst[i] = response; }
            if (t + 1 > j.deadline) { misses[i] += 1; }
            ready.erase(ready.begin() + pick);
        }
    }
    // jobs still unfinished at the horizon whose deadline has passed are misses too
    for (const Job& j : ready) {
        if (j.deadline <= horizon) { misses[static_cast<std::size_t>(j.task)] += 1; }
    }
    std::printf("%s, %s\n", title, p == Policy::RM ? "rate-monotonic" : "EDF");
    std::printf("  first %d units of %d (one character per unit, '.' = idle):\n",
                horizon < 40 ? horizon : 40, horizon);
    for (int s = 0; s < horizon && s < 40; s += 20) {
        std::printf("  t=%3d |%s|\n", s, line.substr(static_cast<std::size_t>(s), 20).c_str());
    }
    for (std::size_t i = 0; i < ts.size(); ++i) {
        std::printf("  task %c  T=%2d C=%2d  finished jobs %2d  worst response %3d  misses %2d\n",
                    ts[i].name, ts[i].period, ts[i].cost, jobs[i], worst[i], misses[i]);
    }
}

double utilisation(const std::vector<Task>& ts)
{
    double u = 0;
    for (const Task& t : ts) { u += static_cast<double>(t.cost) / t.period; }
    return u;
}

int main()
{
    const std::vector<Task> full{{'A', 4, 2}, {'B', 6, 3}};
    std::printf("set 1: U = %.4f (exactly 1)\n", utilisation(full));
    simulate("set 1", full, Policy::RM, 24);
    simulate("set 1", full, Policy::EDF, 24);

    const std::vector<Task> overload{{'A', 5, 2}, {'B', 10, 4}, {'C', 20, 6}};
    std::printf("\nset 2 (overload): U = %.4f\n", utilisation(overload));
    simulate("set 2", overload, Policy::RM, 120);
    simulate("set 2", overload, Policy::EDF, 120);
    return 0;
}
