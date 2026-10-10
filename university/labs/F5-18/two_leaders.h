// two_leaders.h - three servers, one blocked link and one quick reboot of n3.
// fault = true runs the servers with the lazyPersist fault of raft.h.
#pragma once
#include "cluster.h"

inline int twoLeadersStory(bool fault)
{
    using raft::OpType;
    raft::Options opt;
    opt.fixedTimeout = {{1, 150}, {2, 152}, {3, 400}};
    raft::Faults faults;
    faults.lazyPersist = fault;
    sim::NetOptions net;
    net.minDelay = 5;
    net.maxDelay = 5;
    raft::Cluster c(3, 1, opt, faults, net);
    c.at(1, [](raft::Cluster& k) { k.block(1, 2); });
    c.at(156, [](raft::Cluster& k) { k.crash(3); k.restart(3); });
    c.at(250, [](raft::Cluster& k) { k.submit(1, OpType::Put, 'x', 1); k.submit(2, OpType::Put, 'x', 2); });
    c.at(400, [](raft::Cluster& k) { k.unblock(1, 2); });
    c.runUntil(500);
    c.printLogs();
    c.printStates();
    c.printViolations();
    return c.violations().empty() ? 0 : 1;
}
