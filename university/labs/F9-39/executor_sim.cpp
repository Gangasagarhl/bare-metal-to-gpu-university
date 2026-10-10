// executor_sim.cpp - the university's model of executors and callback groups (F9-39).
// Not ROS 2: a discrete-event model with 1 ms steps. Each callback has a cost (ms of CPU work).
// Rules of the model: a free thread takes the oldest ready callback whose group allows it;
// an exclusive group runs at most one of its callbacks at a time; a reentrant group has no limit.
#include <algorithm>
#include <cstdio>
#include <iostream>
#include <map>
#include <string>
#include <vector>

struct Group { bool exclusive = true; int running = 0; };
struct Callback {
    std::string name, group;
    bool timer = true;
    int period = 0, cost = 0;
    int nextDue = 0;                       // timer: next deadline; subscription: next arrival
    bool timerReady = false; int readyAt = 0; bool timerRunning = false;
    std::vector<int> pending;              // subscription: arrival times of queued messages
    int runs = 0, maxLat = 0, missed = 0; long sumLat = 0;
};
struct Job { int cb = -1; int start = 0; int end = 0; };

void simulate(const std::string& title, int threads, std::map<std::string, Group> groups,
              std::vector<Callback> cbs, int horizon, int traceFrom, int traceTo)
{
    std::printf("== scenario %s: %d thread(s)", title.c_str(), threads);
    for (const auto& [n, g] : groups) std::printf(", group %s %s", n.c_str(), g.exclusive ? "exclusive" : "reentrant");
    std::printf("\n");
    for (auto& c : cbs) c.nextDue = c.period;
    std::vector<Job> running(threads);
    std::vector<int> busyMs(threads, 0);
    for (int t = 0; t < horizon; ++t) {
        for (int k = 0; k < threads; ++k) {                       // finish jobs that end now
            Job& j = running[k];
            if (j.cb >= 0 && j.end == t) {
                Callback& c = cbs[j.cb];
                groups[c.group].running--;
                if (c.timer) c.timerRunning = false;
                if (t >= traceFrom && t < traceTo) std::printf("  t=%4d  thread %d  end    %s\n", t, k, c.name.c_str());
                j.cb = -1;
            }
        }
        for (auto& c : cbs) {                                     // release timers and messages
            if (t == c.nextDue) {
                if (c.timer) {
                    if (c.timerReady || c.timerRunning) c.missed++;   // previous one not yet started or still running
                    else { c.timerReady = true; c.readyAt = t; }
                } else {
                    c.pending.push_back(t);
                }
                c.nextDue += c.period;
            }
        }
        for (int k = 0; k < threads; ++k) {                       // free threads pick work
            if (running[k].cb >= 0) continue;
            int best = -1, bestTime = 0;
            for (int i = 0; i < static_cast<int>(cbs.size()); ++i) {
                Callback& c = cbs[i];
                bool ready = c.timer ? c.timerReady : !c.pending.empty();
                if (!ready) continue;
                Group& g = groups[c.group];
                if (g.exclusive && g.running > 0) continue;
                int readyTime = c.timer ? c.readyAt : c.pending.front();
                if (best < 0 || readyTime < bestTime) { best = i; bestTime = readyTime; }
            }
            if (best < 0) continue;
            Callback& c = cbs[best];
            int released = bestTime;
            if (c.timer) { c.timerReady = false; c.timerRunning = true; }
            else c.pending.erase(c.pending.begin());
            groups[c.group].running++;
            running[k] = {best, t, t + c.cost};
            busyMs[k] += c.cost;
            int lat = t - released;
            c.runs++; c.sumLat += lat; c.maxLat = std::max(c.maxLat, lat);
            if (t >= traceFrom && t < traceTo)
                std::printf("  t=%4d  thread %d  start  %-9s (released t=%d, waited %d ms)\n", t, k, c.name.c_str(), released, lat);
        }
    }
    std::printf("  %-9s %-6s %6s %6s %10s %10s %7s\n", "callback", "group", "period", "runs", "mean wait", "max wait", "missed");
    for (const auto& c : cbs)
        std::printf("  %-9s %-6s %6d %6d %8.1f ms %7d ms %7d\n", c.name.c_str(), c.group.c_str(), c.period, c.runs,
                    c.runs ? static_cast<double>(c.sumLat) / c.runs : 0.0, c.maxLat, c.missed);
    for (int k = 0; k < threads; ++k) std::printf("  thread %d busy %d of %d ms\n", k, busyMs[k], horizon);
}

int main()
{
    std::string word;
    std::string title; int threads = 1; int horizon = 1000; int traceFrom = 0, traceTo = 0;
    std::map<std::string, Group> groups;
    std::vector<Callback> cbs;
    while (std::cin >> word) {
        if (word == "scenario") { std::cin >> title >> threads; groups.clear(); cbs.clear(); traceFrom = traceTo = 0; }
        else if (word == "group") { std::string n, kind; std::cin >> n >> kind; groups[n].exclusive = (kind == "exclusive"); }
        else if (word == "timer" || word == "sub") {
            Callback c; c.timer = (word == "timer");
            std::cin >> c.name >> c.period >> c.cost >> c.group;
            cbs.push_back(c);
        }
        else if (word == "trace") std::cin >> traceFrom >> traceTo;
        else if (word == "run") { std::cin >> horizon; simulate(title, threads, groups, cbs, horizon, traceFrom, traceTo); }
    }
    return 0;
}
