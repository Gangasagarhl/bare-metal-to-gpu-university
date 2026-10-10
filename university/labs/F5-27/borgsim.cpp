// borgsim.cpp - four ideas F5-27 takes from the Borg paper, in one small model:
// admission by quota, priority bands with preemption, best-fit placement, and
// resource reclamation (lower-priority work runs in the gap between what production
// tasks reserved and what they use). The university's own model, not Google's code.
#include <algorithm>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

struct Machine { std::string name; int cpu = 0; };

struct Task
{
    std::string name, user, band;   // band: prod or batch
    int prio = 0, limit = 0, usage = 0;
    int submit = 0, work = 0;       // work: minutes still needed; -1 = runs forever (a service)
    int machine = -1, progress = 0, preemptions = 0, done = -1;
};

int prioOf(const std::string& band) { return band == "prod" ? 200 : 100; }

// CPU the machine must keep for the tasks already on it, as seen by `newcomer`.
int committed(const std::vector<Task>& tasks, int m, const Task& newcomer, bool reclaim)
{
    int sum = 0;
    for (const Task& t : tasks) {
        if (t.machine != m) {
            continue;
        }
        // With reclamation, batch work is placed against the USAGE of prod tasks,
        // not their limits; prod work always counts everyone's limits.
        const bool useUsage = reclaim && newcomer.band == "batch" && t.band == "prod";
        sum += useUsage ? t.usage : t.limit;
    }
    return sum;
}

void simulate(const std::string& mode, const std::vector<Machine>& ms, std::vector<Task> tasks,
              std::map<std::string, int> quota, int horizon)
{
    const bool reclaim = mode == "on";
    std::cout << "=== reclamation " << mode << " ===\n";
    std::vector<Task> admitted;
    for (Task& t : tasks) {                           // admission control at submit time
        int& q = quota[t.user + "/" + t.band];
        if (t.limit > q) {
            std::cout << "t=" << t.submit << "  REJECT " << t.name << ": quota of " << t.user << '/'
                      << t.band << " has " << q << " CPU left, task needs " << t.limit << '\n';
            continue;
        }
        q -= t.limit;
        admitted.push_back(t);
    }
    tasks = admitted;
    for (int now = 0; now < horizon; ++now) {
        for (Task& t : tasks) {                       // progress and completion
            if (t.machine >= 0 && t.work > 0 && ++t.progress >= t.work) {
                t.machine = -1;
                t.done = now;
                std::cout << "t=" << now << "  done " << t.name << '\n';
            }
        }
        std::vector<Task*> pending;
        for (Task& t : tasks) {
            if (t.machine < 0 && t.done < 0 && t.submit <= now) {
                pending.push_back(&t);
            }
        }
        std::stable_sort(pending.begin(), pending.end(),
                         [](const Task* a, const Task* b) { return a->prio > b->prio; });
        for (Task* t : pending) {
            int best = -1, bestLeft = 0;              // best fit: least CPU left over
            for (int m = 0; m < static_cast<int>(ms.size()); ++m) {
                const int left = ms[m].cpu - committed(tasks, m, *t, reclaim) - t->limit;
                if (left >= 0 && (best < 0 || left < bestLeft)) {
                    best = m;
                    bestLeft = left;
                }
            }
            if (best < 0 && t->band == "prod") {      // preempt lower-priority tasks
                for (int m = 0; m < static_cast<int>(ms.size()) && best < 0; ++m) {
                    int freeable = 0;
                    for (const Task& o : tasks) {
                        freeable += (o.machine == m && o.prio < t->prio) ? o.limit : 0;
                    }
                    if (ms[m].cpu - committed(tasks, m, *t, reclaim) + freeable < t->limit) {
                        continue;
                    }
                    for (Task& o : tasks) {
                        if (o.machine == m && o.prio < t->prio &&
                            ms[m].cpu - committed(tasks, m, *t, reclaim) < t->limit) {
                            o.machine = -1;
                            o.progress = 0;           // no checkpoint: the work is lost
                            ++o.preemptions;
                            std::cout << "t=" << now << "  PREEMPT " << o.name << " on "
                                      << ms[m].name << " for " << t->name << '\n';
                        }
                    }
                    best = m;
                }
            }
            if (best >= 0) {
                t->machine = best;
                std::cout << "t=" << now << "  place " << t->name << " on " << ms[best].name
                          << '\n';
            }
        }
    }
    int limits = 0, used = 0, capacity = 0, running = 0;
    for (const Machine& m : ms) {
        capacity += m.cpu;
    }
    std::cout << "at t=" << horizon << ":";
    for (const Task& t : tasks) {
        if (t.machine >= 0) {
            ++running;
            limits += t.limit;
            used += t.usage;
        }
        std::cout << ' ' << t.name << '='
                  << (t.done >= 0 ? "done" : t.machine >= 0 ? ms[t.machine].name : "pending");
        if (t.preemptions > 0) {
            std::cout << "(preempted " << t.preemptions << "x)";
        }
    }
    std::cout << "\nrunning tasks " << running << "; CPU reserved by their limits " << limits
              << " of "
              << capacity << "; CPU actually used " << used << " of " << capacity << "\n\n";
}

int main()
{
    std::vector<Machine> ms;
    std::vector<Task> tasks;
    std::map<std::string, int> quota;
    std::vector<std::string> modes;
    int horizon = 60;
    std::string line;
    while (std::getline(std::cin, line)) {
        std::istringstream in(line);
        std::string kind;
        in >> kind;
        if (kind == "machine") {
            Machine m;
            in >> m.name >> m.cpu;
            ms.push_back(m);
        } else if (kind == "quota") {
            std::string user, band;
            int cpu = 0;
            in >> user >> band >> cpu;
            quota[user + "/" + band] = cpu;
        } else if (kind == "task") {                  // name user band limit usage submit work
            Task t;
            in >> t.name >> t.user >> t.band >> t.limit >> t.usage >> t.submit >> t.work;
            t.prio = prioOf(t.band);
            tasks.push_back(t);
        } else if (kind == "reclamation") {
            std::string m;
            in >> m;
            modes.push_back(m);
        } else if (kind == "horizon") {
            in >> horizon;
        }
    }
    for (const std::string& m : modes) {
        simulate(m, ms, tasks, quota, horizon);
    }
    return 0;
}
