// Five servers start as followers; the leader crashes; a new one is elected.
#include "cluster.h"
int main()
{
    raft::Cluster c(5, 7);  // 5 servers, seed 7, default options
    c.at(400, [](raft::Cluster& k) {
        for (int id = 1; id <= 5; ++id) {
            if (k.node(id).role() == raft::Role::Leader) {
                k.crash(id);
            }
        }
    });
    c.runUntil(1000);
    c.printLogs();
    c.printViolations();
    return c.violations().empty() ? 0 : 1;
}
