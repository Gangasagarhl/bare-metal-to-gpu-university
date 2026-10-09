// rta.cpp - F3-39 Listing 4: response-time analysis (RTA) for fixed-priority preemptive
// scheduling, and an ideal (zero-overhead) simulation of the same task set, to compare with
// what uRTOS measured on the emulated board. Times are in tenths of a tick (integers).
#include <cmath>
#include <iostream>
#include <string>
#include <vector>

struct TaskSpec {
    std::string name;
    char letter;
    int priority;   // larger = more urgent
    int period;     // tenths of a tick; deadline = period
    int cost;       // tenths of a tick
};

// R = C_i + sum over more urgent tasks j of ceil(R / T_j) * C_j, iterated to a fixed point.
int responseTime(const std::vector<TaskSpec>& set, const TaskSpec& t, bool verbose)
{
    int r = t.cost;
    for (int iter = 0; iter < 50; ++iter) {
        int next = t.cost;
        std::string terms = std::to_string(t.cost);
        for (const TaskSpec& j : set) {
            if (j.priority > t.priority) {
                const int n = (r + j.period - 1) / j.period;      // ceil(r / T_j)
                next += n * j.cost;
                terms += " + " + std::to_string(n) + "x" + std::to_string(j.cost);
            }
        }
        if (verbose) {
            std::cout << "    R = " << terms << " = " << next << "\n";
        }
        if (next == r || next > t.period) {
            return next;
        }
        r = next;
    }
    return r;
}

void analyse(const std::string& title, const std::vector<TaskSpec>& set)
{
    std::cout << "== " << title << " ==\n";
    double u = 0;
    for (const TaskSpec& t : set) {
        u += static_cast<double>(t.cost) / t.period;
    }
    const double n = static_cast<double>(set.size());
    std::cout << "utilisation U = " << u << ", Liu-Layland bound n(2^(1/n)-1) = "
              << n * (std::pow(2.0, 1.0 / n) - 1.0) << "\n";
    for (const TaskSpec& t : set) {
        std::cout << "  " << t.name << " (priority " << t.priority << ", T = " << t.period / 10.0
                  << ", C = " << t.cost / 10.0 << "):\n";
        const int r = responseTime(set, t, true);
        std::cout << "    -> worst response " << r / 10.0 << " ticks, deadline " << t.period / 10.0
                  << (r <= t.period ? " (met)\n" : " (MISSED)\n");
    }
}

// Ideal preemptive schedule in steps of 0.1 tick; one character per quarter tick (the
// character of the step nearest the quarter's centre), as in the firmware's timeline.
void simulate(const std::vector<TaskSpec>& set, int ticks)
{
    std::vector<int> remaining(set.size(), 0);
    std::string steps;
    for (int s = 0; s < ticks * 10; ++s) {
        for (size_t i = 0; i < set.size(); ++i) {
            if (s % set[i].period == 0) {
                remaining[i] += set[i].cost;              // a new job is released
            }
        }
        int best = -1;
        for (size_t i = 0; i < set.size(); ++i) {
            if (remaining[i] > 0 && (best < 0 || set[i].priority > set[best].priority)) {
                best = static_cast<int>(i);
            }
        }
        steps += best < 0 ? '.' : set[best].letter;
        if (best >= 0) {
            --remaining[best];
        }
    }
    std::cout << "ideal timeline, one character per quarter tick:\n";
    for (int row = 0; row < ticks / 12; ++row) {
        std::cout << (row == 0 ? "ticks  0-11 |" : "ticks 12-23 |");
        for (int q = 0; q < 48; ++q) {
            const int centre = (row * 48 + q) * 10 / 4 + 1;   // 0.125 tick into the quarter
            std::cout << steps[centre] << (q % 4 == 3 ? "|" : "");
        }
        std::cout << "\n";
    }
}

int main()
{
    const std::vector<TaskSpec> rateMonotonic = {
        {"sensor", 'A', 3, 40, 8}, {"control", 'B', 2, 60, 18}, {"filter", 'C', 1, 120, 28}};
    const std::vector<TaskSpec> swapped = {
        {"sensor", 'A', 1, 40, 8}, {"control", 'B', 2, 60, 18}, {"filter", 'C', 3, 120, 28}};
    analyse("rate-monotonic priorities (app.cc)", rateMonotonic);
    simulate(rateMonotonic, 24);
    analyse("filter first (app_v2.cc)", swapped);
    simulate(swapped, 24);
    return 0;
}
