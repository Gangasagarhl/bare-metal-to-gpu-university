// stale_read_story.h - five servers, a partition, and two clients that read.
// fault = true runs the leaders with the readFromLocalState fault of raft.h.
#pragma once
#include "linearizability.h"

inline int staleReadStory(bool fault)
{
    using raft::OpType;
    raft::Options opt;
    opt.fixedTimeout = {{1, 100}, {2, 400}, {3, 300}, {4, 500}, {5, 500}};
    raft::Faults faults;
    faults.readFromLocalState = fault;
    sim::NetOptions net;
    net.minDelay = 5;
    net.maxDelay = 5;
    raft::Cluster c(5, 1, opt, faults, net);
    c.at(200, [](raft::Cluster& k) { k.submit(1, OpType::Put, 'x', 1); });
    c.at(300, [](raft::Cluster& k) { k.partition({1, 2}); });
    c.at(700, [](raft::Cluster& k) { k.submit(3, OpType::Put, 'x', 2); });
    c.at(800, [](raft::Cluster& k) { k.submit(1, OpType::Get, 'x'); });
    c.at(900, [](raft::Cluster& k) { k.submit(3, OpType::Get, 'x'); });
    c.at(1000, [](raft::Cluster& k) { k.heal(); });
    c.runUntil(1100);
    c.printViolations();
    std::printf("client history:\n");
    lin::printHistory(c.history());
    lin::Checker lc(c.history());
    std::vector<int> order;
    if (lc.check(order)) {
        std::printf("linearizable; one valid order:");
        for (int i : order) {
            const auto& h = lc.ops()[i];
            std::printf(" %s%d", h.isPut ? "put " : "get->", h.value);
        }
        std::printf("\n");
        return 0;
    }
    std::printf("NOT linearizable: no order of these operations fits both the answers and real time\n");
    return 1;
}
