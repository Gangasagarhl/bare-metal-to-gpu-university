// hang.cpp - BR-10 Listing 6: the trap "no timeouts".
// Node n2 stalls forever on its first task but keeps its connection open.
// The controller has no deadline, so it waits forever (the lab's time limit stops it).
#include "cluster.hpp"

int main()
{
    mini::Config cfg;
    cfg.nodes[1].stallOnTask = 0;  // n2: the first task it receives ...
    cfg.nodes[1].stallMs = -1;     // ... never finishes
    cfg.timeoutMs = 0;             // the trap: no deadline
    mini::Cluster c(cfg);
    mini::JobResult const r = c.runJob();
    std::printf("finished: %s\n", r.finished ? "yes" : "no");  // never reached
    return 0;
}
