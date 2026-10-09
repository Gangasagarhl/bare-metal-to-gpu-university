// batchsim.cpp - a tiny batch scheduler: a queue of jobs, a table of nodes, and two
// policies (strict priority order, and priority order with backfill). This is the
// university's own model of the ideas in F5-23, not Slurm's code (DS303).
#include <algorithm>
#include <iomanip>
#include <iostream>
#include <set>
#include <sstream>
#include <string>
#include <vector>

struct Node
{
    std::string name;
    int cpus = 0, mem = 0, gpus = 0;           // capacity
    int usedCpus = 0, usedMem = 0, usedGpus = 0;
};

struct Job
{
    std::string id;
    int submit = 0, nodes = 0, cpus = 0, mem = 0, gpus = 0;  // per-node request
    int limit = 0, runtime = 0, priority = 0;   // minutes; runtime is unknown to the scheduler
    int start = -1, end = -1;
    std::vector<int> alloc;                     // node indexes while running
    std::string lastReason;
};

bool fitsFree(const Node& n, const Job& j)
{
    return n.cpus - n.usedCpus >= j.cpus && n.mem - n.usedMem >= j.mem &&
           n.gpus - n.usedGpus >= j.gpus;
}

// First fit: the lowest-numbered nodes with enough free resources, avoiding `avoid`.
std::vector<int> pickNodes(const std::vector<Node>& nodes, const Job& j,
                           const std::set<int>& avoid)
{
    std::vector<int> pick;
    const int n = static_cast<int>(nodes.size());
    for (int i = 0; i < n && static_cast<int>(pick.size()) < j.nodes; ++i) {
        if (avoid.count(i) == 0 && fitsFree(nodes[i], j)) {
            pick.push_back(i);
        }
    }
    if (static_cast<int>(pick.size()) < j.nodes) {
        pick.clear();
    }
    return pick;
}

void take(std::vector<Node>& nodes, const Job& j, int sign)
{
    for (int i : j.alloc) {
        nodes[i].usedCpus += sign * j.cpus;
        nodes[i].usedMem += sign * j.mem;
        nodes[i].usedGpus += sign * j.gpus;
    }
}

// Earliest time and node set for job `head`, assuming running jobs end at their LIMITS.
int reservation(std::vector<Node> nodes, const std::vector<Job>& jobs, const Job& head, int now,
                std::set<int>& resNodes)
{
    std::vector<const Job*> running;
    for (const Job& j : jobs) {
        if (j.start >= 0 && j.end < 0) {
            running.push_back(&j);
        }
    }
    std::sort(running.begin(), running.end(),
              [](const Job* a, const Job* b) { return a->start + a->limit < b->start + b->limit; });
    int t = now;
    for (std::size_t k = 0; k <= running.size(); ++k) {
        std::vector<int> pick = pickNodes(nodes, head, {});
        if (!pick.empty()) {
            resNodes = std::set<int>(pick.begin(), pick.end());
            return t;
        }
        if (k < running.size()) {
            take(nodes, *running[k], -1);           // pretend it ended at its limit
            t = running[k]->start + running[k]->limit;
        }
    }
    return -1;                                   // never, even with every node empty
}

void simulate(const std::string& policy, std::vector<Node> nodes, std::vector<Job> jobs)
{
    std::cout << "=== policy: " << policy << " ===\n";
    for (int now = 0; now <= 10000; ++now) {
        for (Job& j : jobs) {                     // 1. finish jobs whose time is up
            if (j.start >= 0 && j.end < 0 && now >= j.start + std::min(j.runtime, j.limit)) {
                j.end = now;
                take(nodes, j, -1);
                std::cout << "t=" << std::setw(4) << now << "  end    " << j.id
                          << (j.runtime > j.limit ? "  (killed: time limit reached)" : "") << '\n';
            }
        }
        std::vector<Job*> queue;                  // 2. pending jobs, highest priority first
        for (Job& j : jobs) {
            if (j.start < 0 && j.submit <= now) {
                queue.push_back(&j);
            }
        }
        std::stable_sort(queue.begin(), queue.end(),
                         [](const Job* a, const Job* b) { return a->priority > b->priority; });
        bool blocked = false;
        int shadow = -1;
        std::set<int> resNodes;
        for (Job* j : queue) {                    // 3. try to start each one in order
            std::vector<int> pick;
            std::string reason;
            if (!blocked) {
                pick = pickNodes(nodes, *j, {});
                if (pick.empty()) {
                    reason = "waiting for resources";
                    blocked = true;
                    shadow = reservation(nodes, jobs, *j, now, resNodes);
                }
            } else if (policy == "backfill") {
                pick = pickNodes(nodes, *j, {});
                bool endsInTime = shadow >= 0 && now + j->limit <= shadow;
                if (!pick.empty() && !endsInTime) {
                    pick = pickNodes(nodes, *j, resNodes);   // stay off the reserved nodes
                }
                if (pick.empty()) {
                    reason = "waiting: higher-priority job holds a reservation";
                }
            } else {
                reason = "waiting: a higher-priority job is first in line";
            }
            if (!pick.empty()) {
                j->start = now;
                j->alloc = pick;
                take(nodes, *j, +1);
                std::cout << "t=" << std::setw(4) << now << "  start  " << j->id << " on";
                for (int i : pick) {
                    std::cout << ' ' << nodes[i].name;
                }
                std::cout << (blocked ? "  (backfilled)" : "") << '\n';
            } else if (reason != j->lastReason) {
                std::cout << "t=" << std::setw(4) << now << "  pend   " << j->id << ": " << reason
                          << '\n';
                j->lastReason = reason;
            }
        }
        bool anyRunning = false, anyFuture = false;
        for (const Job& j : jobs) {
            anyRunning = anyRunning || (j.start >= 0 && j.end < 0);
            anyFuture = anyFuture || j.submit > now;
        }
        if (!anyRunning && !anyFuture) {
            break;                                // nothing can change any more
        }
    }
    int waitSum = 0, done = 0, last = 0;
    std::cout << "job  submit start   end  wait\n";
    for (const Job& j : jobs) {
        if (j.start < 0) {
            std::cout << std::setw(4) << j.id << std::setw(7) << j.submit
                      << "  never started (still pending)\n";
            continue;
        }
        std::cout << std::setw(4) << j.id << std::setw(7) << j.submit << std::setw(6) << j.start
                  << std::setw(6) << j.end << std::setw(6) << j.start - j.submit << '\n';
        waitSum += j.start - j.submit;
        ++done;
        last = std::max(last, j.end);
    }
    if (done > 0) {
        std::cout << "finished jobs " << done << ", average wait " << waitSum / done
                  << " min, last job ended at t=" << last << '\n';
    }
    std::cout << "node states at the end (capacity: CPUs/GiB/GPUs):\n";
    for (const Node& n : nodes) {
        std::cout << "  " << n.name << "  " << n.cpus << '/' << n.mem << '/' << n.gpus << "  "
                  << (n.usedCpus == 0 ? "idle" : "allocated") << '\n';
    }
    std::cout << '\n';
}

int main()
{
    std::vector<Node> nodes;
    std::vector<Job> jobs;
    std::vector<std::string> policies;
    std::string line;
    while (std::getline(std::cin, line)) {
        std::istringstream in(line);
        std::string kind;
        in >> kind;
        if (kind == "node") {
            Node n;
            in >> n.name >> n.cpus >> n.mem >> n.gpus;
            nodes.push_back(n);
        } else if (kind == "job") {
            Job j;
            in >> j.id >> j.submit >> j.nodes >> j.cpus >> j.mem >> j.gpus >> j.limit >> j.runtime
               >> j.priority;
            jobs.push_back(j);
        } else if (kind == "policy") {
            std::string p;
            in >> p;
            policies.push_back(p);
        }
    }
    for (const std::string& p : policies) {
        simulate(p, nodes, jobs);
    }
    return 0;
}
