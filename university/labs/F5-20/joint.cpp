// Growing a cluster from {1,2,3} to {1,2,3,4,5}, then removing the leader,
// with joint consensus (Raft paper, section 6).
#include "cluster.h"
int main()
{
    using raft::OpType;
    raft::Options opt;
    opt.fixedTimeout = {{1, 100}};  // n1 becomes the first leader; others random
    sim::NetOptions net;
    net.minDelay = 5;
    net.maxDelay = 5;
    raft::Cluster c(3, 11, opt, {}, net);
    c.addSpareServer(4);
    c.addSpareServer(5);
    c.at(200, [](raft::Cluster& k) { k.submit(1, OpType::Put, 'x', 1); });
    c.at(300, [](raft::Cluster& k) { k.changeConfig(1, {1, 2, 3, 4, 5}); });
    c.at(310, [](raft::Cluster& k) { k.submit(1, OpType::Put, 'x', 2); });
    c.at(500, [](raft::Cluster& k) { k.changeConfig(1, {2, 3, 4, 5}); });
    c.runUntil(1100);
    c.printLogs();
    c.printStates();
    c.printViolations();
    return c.violations().empty() ? 0 : 1;
}
