// timeouts.cpp - BR-10 Listing 7: the same faults, with a deadline on every task.
// n2 stalls for 1.2 s on its first task (slow, not dead); n3 dies on its second task.
// The controller suspects n2 after 0.5 s, notices n3's closed connection, gives both
// tasks to healthy nodes, and still produces the laptop's answer.
#include "cluster.hpp"

int main()
{
    mini::Config cfg;
    cfg.nodes[1].stallOnTask = 0;
    cfg.nodes[1].stallMs = 1200;  // n2: slow for 1.2 s, then answers after all
    cfg.nodes[2].dieOnTask = 1;   // n3: sudden death on its second task
    cfg.timeoutMs = 500;          // the fix: a deadline for every task
    cfg.drainMs = 2000;           // listen for late answers after the job
    job::Part const answer = job::countPrimes(0, cfg.jobEnd);
    bool ok = false;
    {
        mini::Cluster c(cfg);
        mini::JobResult const r = c.runJob();
        ok = r.finished && r.total.count == answer.count && r.total.sum == answer.sum;
        std::printf("job finished: %s in %.0f ms; same answer as the laptop: %s\n",
                    r.finished ? "yes" : "no", r.seconds * 1e3, ok ? "yes" : "NO");
        for (std::size_t i = 0; i < r.perNode.size(); ++i) {
            std::printf("  n%zu finished %d tasks\n", i + 1, r.perNode[i].tasks);
        }
    }  // the destructor fences the suspected node and collects every node's exit
    return ok ? 0 : 1;
}
