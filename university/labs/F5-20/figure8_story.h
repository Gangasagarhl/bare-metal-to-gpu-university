// figure8_story.h - five servers replay the situation of Figure 8 of the Raft paper.
// fault = true runs the leaders with the commitOldTermByCount fault of raft.h.
#pragma once
#include "cluster.h"

inline int figure8Story(bool fault)
{
    using raft::OpType;
    raft::Options opt;
    opt.noopOnElection = false;  // as in the paper's figure: no entry of the new term
    opt.fixedTimeout = {{1, 100}, {2, 1500}, {3, 1500}, {4, 1500}, {5, 200}};
    raft::Faults faults;
    faults.commitOldTermByCount = fault;
    sim::NetOptions net;
    net.minDelay = 5;
    net.maxDelay = 5;
    raft::Cluster c(5, 1, opt, faults, net);

    c.at(200, [](raft::Cluster& k) { k.submit(1, OpType::Put, 'x', 1); });
    c.at(300, [](raft::Cluster& k) {
        k.block(1, 3); k.block(1, 4); k.block(1, 5);  // n1 can reach only n2
        k.submit(1, OpType::Put, 'x', 2);
    });
    c.at(320, [](raft::Cluster& k) { k.crash(1); });
    c.at(600, [](raft::Cluster& k) {
        k.isolate(5);
        k.submit(5, OpType::Put, 'x', 3);
    });
    c.at(620, [](raft::Cluster& k) { k.crash(5); });
    c.at(650, [](raft::Cluster& k) { k.reconnect(1); k.restart(1); });
    c.at(1000, [](raft::Cluster& k) { k.crash(1); });
    c.at(1010, [](raft::Cluster& k) { k.reconnect(5); k.restart(5); });
    c.at(1500, [](raft::Cluster& k) { k.submit(5, OpType::Put, 'y', 4); });
    c.runUntil(1700);
    c.printLogs();
    c.printStates();
    c.printViolations();
    return c.violations().empty() ? 0 : 1;
}
