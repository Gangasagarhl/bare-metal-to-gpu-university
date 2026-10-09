// replication_story.h - three servers: a follower falls behind, the leader is cut
// off with an uncommitted entry, a new leader repairs both logs.
// fault = true runs the followers with the skipPrevLogCheck fault of raft.h.
#pragma once
#include "cluster.h"

inline int replicationStory(bool fault)
{
    using raft::OpType;
    raft::Options opt;
    opt.fixedTimeout = {{1, 100}, {2, 300}, {3, 2000}};
    sim::NetOptions net;
    net.minDelay = 5;
    net.maxDelay = 5;
    raft::Faults faults;
    faults.skipPrevLogCheck = fault;
    raft::Cluster c(3, 1, opt, faults, net);
    c.at(200, [](raft::Cluster& k) {
        k.isolate(3);
        k.submit(1, OpType::Put, 'a', 1);
        k.submit(1, OpType::Put, 'b', 2);
        k.submit(1, OpType::Put, 'c', 3);
    });
    c.at(500, [](raft::Cluster& k) {
        k.isolate(1);
        k.submit(1, OpType::Put, 'd', 4);
    });
    c.at(600, [](raft::Cluster& k) { k.reconnect(3); });
    c.at(900, [](raft::Cluster& k) { k.submit(2, OpType::Put, 'e', 5); });
    c.at(1000, [](raft::Cluster& k) { k.reconnect(1); });
    c.runUntil(1200);
    c.printLogs();
    c.printStates();
    c.printViolations();
    return c.violations().empty() ? 0 : 1;
}
