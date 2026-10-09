// gpupack.cpp - placing GPU jobs on GPU nodes with three policies, and what each does
// to fragmentation and topology. Invented cluster: 4 nodes x 8 GPUs, each node in two
// "islands" of 4 GPUs (GPUs 0-3 and 4-7). The university's own model (F5-26, DS303).
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

constexpr int kNodes = 4;
constexpr int kGpus = 8;
constexpr int kIsland = 4;

struct Job
{
    char id = '?';
    int arrive = 0, duration = 0, gpus = 0, nodes = 1;  // gpus per node
    int start = -1;
    std::vector<std::pair<int, int>> held;             // (node, gpu)
    std::string lastMap;                               // last cluster map printed for it
};

using Cluster = std::vector<std::string>;              // one string per node: '.' = free

int freeOn(const std::string& node)
{
    int n = 0;
    for (char c : node) {
        n += c == '.' ? 1 : 0;
    }
    return n;
}

// Choose GPUs on one node: with `topo`, try to stay inside one island first.
std::vector<int> chooseGpus(const std::string& node, int want, bool topo)
{
    std::vector<int> pick;
    if (topo && want <= kIsland) {
        for (int island = 0; island < kGpus / kIsland && pick.empty(); ++island) {
            std::vector<int> inIsland;
            for (int g = island * kIsland; g < (island + 1) * kIsland; ++g) {
                if (node[g] == '.') {
                    inIsland.push_back(g);
                }
            }
            if (static_cast<int>(inIsland.size()) >= want) {
                pick.assign(inIsland.begin(), inIsland.begin() + want);
            }
        }
        if (!pick.empty()) {
            return pick;
        }
    }
    for (int g = 0; g < kGpus && static_cast<int>(pick.size()) < want; ++g) {
        if (node[g] == '.') {
            pick.push_back(g);
        }
    }
    return pick;
}

bool place(Cluster& c, Job& j, const std::string& policy)
{
    std::vector<int> chosen;                           // nodes, in preference order
    for (int pass = 0; pass < j.nodes; ++pass) {
        int best = -1;
        for (int n = 0; n < kNodes; ++n) {
            bool used = false;
            for (int u : chosen) {
                used = used || u == n;
            }
            const int f = freeOn(c[n]);
            if (used || f < j.gpus) {
                continue;
            }
            const bool better = best < 0 ||
                (policy == "spread" ? f > freeOn(c[best]) : f < freeOn(c[best]));
            if (better) {
                best = n;
            }
        }
        if (best < 0) {
            return false;                              // all-or-nothing: place nothing
        }
        chosen.push_back(best);
    }
    for (int n : chosen) {
        for (int g : chooseGpus(c[n], j.gpus, policy == "pack+topology")) {
            c[n][g] = j.id;
            j.held.emplace_back(n, g);
        }
    }
    return true;
}

void run(const std::string& policy, std::vector<Job> jobs)
{
    std::cout << "=== policy: " << policy << " ===\n";
    Cluster c(kNodes, std::string(kGpus, '.'));
    int fragWaits = 0, crossIsland = 0, totalWait = 0;
    for (int t = 0; t <= 60; ++t) {
        for (Job& j : jobs) {                          // jobs that finish now give GPUs back
            if (j.start >= 0 && t == j.start + j.duration) {
                for (auto [n, g] : j.held) {
                    c[n][g] = '.';
                }
            }
        }
        for (Job& j : jobs) {                          // waiting jobs in arrival order
            if (j.start >= 0 || j.arrive > t) {
                continue;
            }
            if (place(c, j, policy)) {
                j.start = t;
                totalWait += t - j.arrive;
                bool islands[2] = {false, false};
                for (auto [n, g] : j.held) {
                    islands[g / kIsland] = true;
                }
                if (j.nodes == 1 && j.gpus <= kIsland && islands[0] && islands[1]) {
                    ++crossIsland;
                    std::cout << "t=" << t << "  job " << j.id << " (" << j.gpus
                              << " GPUs) placed ACROSS the two islands of a node\n";
                }
                continue;
            }
            int freeTotal = 0;
            for (const std::string& node : c) {
                freeTotal += freeOn(node);
            }
            std::string map;
            for (const std::string& node : c) {
                map += " [" + node + "]";
            }
            if (freeTotal >= j.gpus * j.nodes) {
                ++fragWaits;
                if (map != j.lastMap) {                // print only when the picture changes
                    std::cout << "t=" << t << "  job " << j.id << " waits for " << j.nodes << " x "
                              << j.gpus << " GPUs although " << freeTotal << " GPUs are free:"
                              << map << '\n';
                    j.lastMap = map;
                }
            }
        }
    }
    std::cout << "start times:";
    for (const Job& j : jobs) {
        std::cout << ' ' << j.id << '=' << j.start;
    }
    std::cout << "\nminute-waits with enough free GPUs in total: " << fragWaits
              << "; small jobs split across islands: " << crossIsland
              << "; total wait: " << totalWait << " min\n\n";
}

int main()
{
    std::vector<Job> jobs;
    std::vector<std::string> policies;
    std::string line;
    while (std::getline(std::cin, line)) {
        std::istringstream in(line);
        if (line.rfind("policy ", 0) == 0) {           // optional: run only these policies
            policies.push_back(line.substr(7));
            continue;
        }
        Job j;
        const bool parsed =
            static_cast<bool>(in >> j.id >> j.arrive >> j.duration >> j.gpus >> j.nodes);
        if (line.empty() || line[0] == '#' || !parsed) {
            continue;
        }
        jobs.push_back(j);
    }
    if (policies.empty()) {
        policies = {"spread", "pack", "pack+topology"};
    }
    for (const std::string& p : policies) {
        run(p, jobs);
    }
    return 0;
}
