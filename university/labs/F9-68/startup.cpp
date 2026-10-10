// startup.cpp - F9-68: simulate the start-up of a robot's software units and print the timeline.
// Input (stdin), one unit per line (the course's own format, modelled on service managers):
//   unit NAME START_MS READY_MS [needs=A,B] [after=C,D] [fail]
//     START_MS : time from launch until the process runs (load, initialise)
//     READY_MS : time from running until the unit reports "ready" (devices found, peers seen)
//     needs=   : launch only after these units are READY; if one fails, this unit is blocked
//     after=   : launch only after these units are RUNNING (ordering only: no readiness, no failure)
//     fail     : this unit never becomes ready (fault injection)
// Output: units in launch order with launch / running / ready times, the last unit to be ready,
// and the critical path of the target unit robot_ready. Cycles are reported, not run.
#include <algorithm>
#include <iomanip>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

struct Unit {
    std::string name;
    long start_ms = 0, ready_ms = 0;
    std::vector<std::string> needs, after;
    bool fail = false;
    // results
    long launch = -1, running = -1, ready = -1;
    std::string blocked_by, decided_by;
};

std::vector<std::string> split(const std::string& s)
{
    std::vector<std::string> out;
    std::stringstream in(s);
    std::string item;
    while (std::getline(in, item, ',')) {
        out.push_back(item);
    }
    return out;
}

int main()
{
    std::vector<Unit> units;
    std::map<std::string, size_t> idx;
    std::string line;
    while (std::getline(std::cin, line)) {
        std::istringstream in(line);
        std::string kind;
        in >> kind;
        if (kind != "unit") {
            continue;
        }
        Unit u;
        in >> u.name >> u.start_ms >> u.ready_ms;
        std::string w;
        while (in >> w) {
            if (w.rfind("needs=", 0) == 0) {
                u.needs = split(w.substr(6));
            } else if (w.rfind("after=", 0) == 0) {
                u.after = split(w.substr(6));
            } else if (w == "fail") {
                u.fail = true;
            }
        }
        idx[u.name] = units.size();
        units.push_back(u);
    }

    // Kahn's algorithm: a unit can be resolved once everything it waits for is resolved.
    std::vector<int> waiting(units.size(), 0);
    std::vector<std::vector<size_t>> waiters(units.size());
    for (size_t i = 0; i < units.size(); ++i) {
        for (const auto* list : {&units[i].needs, &units[i].after}) {
            for (const auto& d : *list) {
                auto it = idx.find(d);
                if (it == idx.end()) {
                    std::cout << "error: " << units[i].name << " waits for unknown unit " << d << '\n';
                    return 2;
                }
                ++waiting[i];
                waiters[it->second].push_back(i);
            }
        }
    }
    std::vector<size_t> ready_list, order;
    for (size_t i = 0; i < units.size(); ++i) {
        if (waiting[i] == 0) {
            ready_list.push_back(i);
        }
    }
    while (!ready_list.empty()) {
        size_t i = ready_list.back();
        ready_list.pop_back();
        order.push_back(i);
        Unit& u = units[i];
        long launch = 0;
        for (const auto& d : u.needs) {
            const Unit& dep = units[idx[d]];
            if (dep.ready < 0) {
                u.blocked_by = dep.name;
            } else if (dep.ready >= launch) {
                launch = dep.ready;
                u.decided_by = dep.name + " ready";
            }
        }
        for (const auto& d : u.after) {
            const Unit& dep = units[idx[d]];
            if (dep.running >= launch) {
                launch = dep.running;
                u.decided_by = dep.name + " running";
            }
        }
        if (u.blocked_by.empty()) {
            u.launch = launch;
            u.running = launch + u.start_ms;
            u.ready = u.fail ? -1 : u.running + u.ready_ms;
        }
        for (size_t w : waiters[i]) {
            if (--waiting[w] == 0) {
                ready_list.push_back(w);
            }
        }
    }
    if (order.size() != units.size()) {
        std::cout << "error: dependency cycle among:";
        for (size_t i = 0; i < units.size(); ++i) {
            if (waiting[i] > 0) {
                std::cout << ' ' << units[i].name;
            }
        }
        std::cout << '\n';
        return 1;
    }

    std::vector<size_t> by_launch = order;
    std::stable_sort(by_launch.begin(), by_launch.end(), [&](size_t a, size_t b) {
        long la = units[a].launch < 0 ? 1L << 40 : units[a].launch;
        long lb = units[b].launch < 0 ? 1L << 40 : units[b].launch;
        return la < lb;
    });
    std::cout << std::left << std::setw(17) << "unit" << std::right << std::setw(8) << "launch"
              << std::setw(9) << "running" << std::setw(8) << "ready" << "  launched when\n";
    long last = 0;
    std::string last_name;
    for (size_t i : by_launch) {
        const Unit& u = units[i];
        std::cout << std::left << std::setw(17) << u.name << std::right;
        if (!u.blocked_by.empty()) {
            std::cout << "  BLOCKED: needs " << u.blocked_by << ", which never became ready\n";
            continue;
        }
        std::cout << std::setw(8) << u.launch << std::setw(9) << u.running << std::setw(8)
                  << (u.ready < 0 ? std::string("FAILED") : std::to_string(u.ready)) << "  "
                  << (u.decided_by.empty() ? "at boot" : u.decided_by) << '\n';
        if (u.ready > last) {
            last = u.ready;
            last_name = u.name;
        }
    }
    std::cout << "last unit ready: " << last_name << " at " << last << " ms\n";
    // the critical path of the target "robot_ready" if there is one, else of the last unit
    std::string at = idx.count("robot_ready") ? "robot_ready" : last_name;
    if (units[idx[at]].ready < 0) {
        std::cout << at << " never became ready\n";
        return 1;
    }
    std::cout << at << " ready at " << units[idx[at]].ready << " ms; critical path:";
    std::vector<std::string> path;
    while (!at.empty()) {
        path.push_back(at);
        const std::string& d = units[idx[at]].decided_by;
        at = d.substr(0, d.find(' '));
    }
    for (auto it = path.rbegin(); it != path.rend(); ++it) {
        std::cout << (it == path.rbegin() ? " " : " -> ") << *it;
    }
    std::cout << '\n';
    return 0;
}
