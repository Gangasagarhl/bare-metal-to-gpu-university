// fuzz.h - one randomized fault-injection run: five servers, three clients,
// crashes, restarts and partitions chosen from the seed; then every safety
// property and the linearizability of the clients' history are checked.
#pragma once
#include <string>
#include <vector>

#include "cluster.h"
#include "linearizability.h"

struct RunResult
{
    std::vector<std::string> violations;  // Raft safety properties
    bool linearizable = true;
    int operations = 0;  // client operations started
    int answered = 0;    // ... that got a reply
    int leaders = 0;     // terms that had a leader
};

enum class Nemesis
{
    Random,       // every 150 ms: crash, restart, partition or heal random servers
    LeaderHunter  // every 50 ms: crash or isolate whoever is leader right now
};

inline int currentLeader(raft::Cluster& k)
{
    for (int id = 1; id <= 5; ++id) {
        if (k.node(id).up() && k.node(id).role() == raft::Role::Leader) {
            return id;
        }
    }
    return 0;
}

inline RunResult fuzzOne(std::uint64_t seed, raft::Faults faults, bool noop = true,
                         Nemesis nemesis = Nemesis::Random)
{
    raft::Options opt;
    opt.noopOnElection = noop;
    sim::NetOptions net;
    net.minDelay = 1;
    net.maxDelay = 20;      // random delays reorder messages
    net.dropPercent = 2;
    net.duplicatePercent = 1;
    raft::Cluster c(5, seed, opt, faults, net);
    c.traceLevel = raft::Cluster::Trace::None;
    c.startClients(3, 50);

    sim::Rng plan(seed * 7919 + 1);  // the fault schedule has its own generator
    for (raft::Time t = 300; nemesis == Nemesis::LeaderHunter && t < 3000; t += 50) {
        const int what = static_cast<int>(plan.range(0, 4));
        const int a = static_cast<int>(plan.range(1, 5));
        c.at(t, [what, a](raft::Cluster& k) {
            const int leader = currentLeader(k);
            if (what == 0 && leader != 0) {
                k.crash(leader);
            } else if (what == 1) {  // bring back one crashed server
                for (int id = 1; id <= 5; ++id) {
                    if (!k.node(id).up()) {
                        k.restart(id);
                        break;
                    }
                }
            } else if (what == 2 && leader != 0) {
                k.isolate(leader);
            } else if (what == 3) {
                k.heal();
            } else if (what == 4) {
                k.crash(a);
            }
        });
    }
    for (raft::Time t = 300; nemesis == Nemesis::Random && t < 3000; t += 150) {
        const int what = static_cast<int>(plan.range(0, 5));
        const int a = static_cast<int>(plan.range(1, 5));
        const int b = static_cast<int>(plan.range(1, 5));
        c.at(t, [what, a, b](raft::Cluster& k) {
            if (what == 0) {
                k.crash(a);
            } else if (what == 1) {
                k.restart(a);
            } else if (what == 2) {
                k.partition(a == b ? std::vector<int>{a} : std::vector<int>{a, b});
            } else if (what == 3) {
                k.heal();
            }  // 4, 5: leave the system alone for a while
        });
    }
    c.at(3000, [](raft::Cluster& k) {  // end: repair everything, let it settle
        k.heal();
        for (int id = 1; id <= 5; ++id) {
            if (!k.node(id).up()) {
                k.restart(id);
            }
        }
    });
    c.runUntil(4000);

    RunResult r;
    r.violations = c.violations();
    lin::Checker lc(c.history());
    std::vector<int> order;
    r.linearizable = !lc.tooBig() && lc.check(order);
    r.operations = static_cast<int>(c.history().size());
    for (const auto& h : c.history()) {
        r.answered += h.response >= 0;
    }
    r.leaders = c.leaderCount();
    return r;
}
