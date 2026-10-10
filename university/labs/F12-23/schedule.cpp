// schedule.cpp - a plan as a range, not a date.
// Reads tasks with three-point estimates (low, likely, high, in working days) and
// their dependencies, then (1) computes the single-date plan that uses only the
// "likely" values and (2) runs a seeded Monte Carlo simulation in which every task
// takes a duration drawn from a triangular distribution over [low, high] with its
// peak at "likely". Prints percentiles of the finish day and how often each task
// was on the critical path. Our own teaching model (F12-23), not a standard tool.
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <map>
#include <random>
#include <sstream>
#include <string>
#include <vector>

struct Task
{
    std::string id;
    double low = 0, likely = 0, high = 0;
    std::vector<std::size_t> deps;  // indices of tasks that must finish first
};

// Inverse of the triangular cumulative distribution: maps u in (0,1) to a duration.
double triangular(double a, double m, double b, double u)
{
    if (b <= a) {
        return a;
    }
    const double f = (m - a) / (b - a);
    if (u < f) {
        return a + std::sqrt(u * (b - a) * (m - a));
    }
    return b - std::sqrt((1.0 - u) * (b - a) * (b - m));
}

// Earliest finish of every task for given durations; returns the project end.
double forward(const std::vector<Task>& tasks, const std::vector<double>& dur,
               std::vector<double>& finish)
{
    double end = 0;
    for (std::size_t i = 0; i < tasks.size(); ++i) {
        double start = 0;
        for (std::size_t d : tasks[i].deps) {
            start = std::max(start, finish[d]);
        }
        finish[i] = start + dur[i];
        end = std::max(end, finish[i]);
    }
    return end;
}

// Walks back from the last task to finish, always through the latest-finishing
// predecessor: that chain is the critical path for these durations.
std::vector<std::size_t> criticalPath(const std::vector<Task>& tasks,
                                      const std::vector<double>& finish)
{
    std::size_t cur = 0;
    for (std::size_t i = 1; i < tasks.size(); ++i) {
        if (finish[i] > finish[cur]) {
            cur = i;
        }
    }
    std::vector<std::size_t> path{cur};
    while (!tasks[cur].deps.empty()) {
        std::size_t best = tasks[cur].deps.front();
        for (std::size_t d : tasks[cur].deps) {
            if (finish[d] > finish[best]) {
                best = d;
            }
        }
        cur = best;
        path.push_back(cur);
    }
    std::reverse(path.begin(), path.end());
    return path;
}

double percentile(const std::vector<double>& sorted, double p)
{
    // nearest-rank percentile: the smallest value with at least p of the data at or below it
    const auto n = sorted.size();
    auto rank = static_cast<std::size_t>(std::ceil(p * static_cast<double>(n)));
    rank = std::clamp<std::size_t>(rank, 1, n);
    return sorted[rank - 1];
}

int main()
{
    std::vector<Task> tasks;
    std::map<std::string, std::size_t> index;
    int trials = 10000;
    std::uint32_t seed = 1;
    std::string line;
    while (std::getline(std::cin, line)) {
        std::istringstream in(line);
        std::string word;
        if (!(in >> word) || word[0] == '#') {
            continue;
        }
        if (word == "trials") {
            in >> trials;
        } else if (word == "seed") {
            in >> seed;
        } else if (word == "task") {
            Task t;
            in >> t.id >> t.low >> t.likely >> t.high;
            if (!in || !(t.low <= t.likely && t.likely <= t.high)) {
                std::cout << "task " << t.id << ": need low <= likely <= high\n";
                return 2;
            }
            std::string after;
            if (in >> after && after == "after") {
                std::string dep;
                while (in >> dep) {
                    auto it = index.find(dep);
                    if (it == index.end()) {
                        std::cout << "task " << t.id << ": unknown dependency " << dep
                                  << " (list tasks after the tasks they depend on)\n";
                        return 2;
                    }
                    t.deps.push_back(it->second);
                }
            }
            index[t.id] = tasks.size();
            tasks.push_back(t);
        }
    }
    if (tasks.empty() || trials < 1) {
        std::cout << "no tasks or no trials\n";
        return 2;
    }

    std::cout << std::fixed << std::setprecision(1);
    std::cout << "task        low  likely  high  mean  after\n";
    double sumLikely = 0, sumMean = 0;
    for (const Task& t : tasks) {
        const double mean = (t.low + t.likely + t.high) / 3.0;
        sumLikely += t.likely;
        sumMean += mean;
        std::cout << std::left << std::setw(10) << t.id << std::right << std::setw(5) << t.low
                  << std::setw(8) << t.likely << std::setw(6) << t.high << std::setw(6) << mean
                  << "  ";
        for (std::size_t d : t.deps) {
            std::cout << tasks[d].id << ' ';
        }
        std::cout << '\n';
    }
    std::cout << "effort if every task takes its likely value: " << sumLikely << " days\n";
    std::cout << "effort, sum of the mean durations:           " << sumMean << " days\n\n";

    // 1. The single-date plan: every task takes exactly its likely value.
    std::vector<double> dur(tasks.size()), finish(tasks.size());
    for (std::size_t i = 0; i < tasks.size(); ++i) {
        dur[i] = tasks[i].likely;
    }
    const double planEnd = forward(tasks, dur, finish);
    std::cout << "single-date plan (all likely values): day " << planEnd << "\n  critical path:";
    for (std::size_t i : criticalPath(tasks, finish)) {
        std::cout << ' ' << tasks[i].id;
    }
    std::cout << "\n\n";

    // 2. Monte Carlo: the same network with uncertain durations.
    std::mt19937 gen(seed);
    std::vector<double> ends;
    std::vector<int> onPath(tasks.size(), 0);
    int meetsPlan = 0;
    for (int k = 0; k < trials; ++k) {
        for (std::size_t i = 0; i < tasks.size(); ++i) {
            const double u = (static_cast<double>(gen()) + 0.5) / 4294967296.0;
            dur[i] = triangular(tasks[i].low, tasks[i].likely, tasks[i].high, u);
        }
        const double end = forward(tasks, dur, finish);
        ends.push_back(end);
        if (end <= planEnd) {
            ++meetsPlan;
        }
        for (std::size_t i : criticalPath(tasks, finish)) {
            ++onPath[i];
        }
    }
    std::sort(ends.begin(), ends.end());
    double mean = 0;
    for (double e : ends) {
        mean += e;
    }
    mean /= static_cast<double>(ends.size());
    std::cout << "Monte Carlo, " << trials << " trials, seed " << seed << ":\n";
    std::cout << "  finish day  P10 " << percentile(ends, 0.10)
              << "  P50 " << percentile(ends, 0.50)
              << "  P80 " << percentile(ends, 0.80) << "  P95 " << percentile(ends, 0.95)
              << "  mean " << mean << '\n';
    std::cout << "  trials finishing by the single-date plan (day " << planEnd << "): "
              << 100.0 * meetsPlan / trials << " %\n";
    std::cout << "  criticality (share of trials on the critical path):\n";
    for (std::size_t i = 0; i < tasks.size(); ++i) {
        std::cout << "    " << std::left << std::setw(10) << tasks[i].id << std::right
                  << std::setw(6) << 100.0 * onPath[i] / trials << " %\n";
    }
    return 0;
}
