// reconcile.cpp - a toy control plane in the style F5-25 describes: one shared store of
// objects, and independent control loops that each compare desired state with observed
// state and act on the difference. The university's own model, not Kubernetes code.
#include <iostream>
#include <map>
#include <string>
#include <vector>

struct Pod
{
    std::string name;
    std::string app;           // label app=<value>
    int cpuMilli = 0;          // CPU request in thousandths of a CPU
    std::string node;          // empty until the scheduler binds it
    std::string phase;         // Pending, Running, Deleted
};

struct NodeObj
{
    std::string name;
    int cpuMilli = 0;
    int lastHeartbeat = 0;
    bool ready = true;
    bool alive = true;         // ground truth, invisible to the control plane
};

struct Store                   // stands in for the API server and its database
{
    std::map<std::string, int> desiredReplicas;   // per app
    std::vector<Pod> pods;
    std::vector<NodeObj> nodes;
    int nextPod = 1;
};

const int kRequest = 1500;     // every pod of the app asks for 1.5 CPU
const int kGrace = 3;          // ticks without heartbeat before a node is NotReady

void log(int t, const std::string& who, const std::string& what)
{
    std::cout << "t=" << (t < 10 ? " " : "") << t << "  " << who << ": " << what << '\n';
}

void replicaController(Store& s, int t)
{
    for (const auto& [app, want] : s.desiredReplicas) {
        int have = 0;
        for (const Pod& p : s.pods) {
            have += (p.app == app && p.phase != "Deleted") ? 1 : 0;
        }
        for (; have < want; ++have) {
            Pod p{app + "-" + std::to_string(s.nextPod++), app, kRequest, "", "Pending"};
            log(t, "replica-controller", "want " + std::to_string(want) + ", create " + p.name);
            s.pods.push_back(p);
        }
        for (auto it = s.pods.rbegin(); it != s.pods.rend() && have > want; ++it) {
            if (it->app == app && it->phase != "Deleted") {
                it->phase = "Deleted";
                --have;
                log(t, "replica-controller",
                    "want " + std::to_string(want) + ", delete " + it->name);
            }
        }
    }
}

int freeMilli(const Store& s, const NodeObj& n)
{
    int used = 0;
    for (const Pod& p : s.pods) {
        used += (p.node == n.name && p.phase != "Deleted") ? p.cpuMilli : 0;
    }
    return n.cpuMilli - used;
}

void scheduler(Store& s, int t)
{
    for (Pod& p : s.pods) {
        if (p.phase != "Pending" || !p.node.empty()) {
            continue;
        }
        const NodeObj* best = nullptr;          // filter: Ready and enough free CPU;
        for (const NodeObj& n : s.nodes) {      // score: the most free CPU wins
            if (n.ready && freeMilli(s, n) >= p.cpuMilli &&
                (best == nullptr || freeMilli(s, n) > freeMilli(s, *best))) {
                best = &n;
            }
        }
        if (best == nullptr) {
            log(t, "scheduler", p.name + " unschedulable: no Ready node has " +
                                    std::to_string(p.cpuMilli) + "m CPU free");
            continue;
        }
        p.node = best->name;
        log(t, "scheduler", "bind " + p.name + " to " + best->name);
    }
}

void nodeAgents(Store& s, int t)
{
    for (NodeObj& n : s.nodes) {
        if (!n.alive) {
            continue;                            // a dead node reports nothing
        }
        n.lastHeartbeat = t;
        for (Pod& p : s.pods) {
            if (p.node == n.name && p.phase == "Pending") {
                p.phase = "Running";
                log(t, "agent on " + n.name, "start " + p.name);
            }
        }
    }
}

void nodeController(Store& s, int t)
{
    for (NodeObj& n : s.nodes) {
        const bool fresh = t - n.lastHeartbeat < kGrace;
        if (n.ready && !fresh) {
            n.ready = false;
            log(t, "node-controller", n.name + " NotReady (no heartbeat since t=" +
                                          std::to_string(n.lastHeartbeat) + ")");
            for (Pod& p : s.pods) {
                if (p.node == n.name && p.phase != "Deleted") {
                    p.phase = "Deleted";
                    log(t, "node-controller", "evict " + p.name);
                }
            }
        } else if (!n.ready && fresh) {
            n.ready = true;
            log(t, "node-controller", n.name + " Ready again");
        }
    }
}

int main()
{
    Store s;
    for (const char* name : {"k1", "k2", "k3"}) {
        s.nodes.push_back(NodeObj{name, 4000, 0, true, true});
    }
    for (int t = 0; t <= 20; ++t) {
        if (t == 0) { s.desiredReplicas["web"] = 3; log(t, "user", "desired replicas of web = 3"); }
        if (t == 4) { s.desiredReplicas["web"] = 5; log(t, "user", "desired replicas of web = 5"); }
        if (t == 7) { s.nodes[1].alive = false; log(t, "world", "k2 loses power, nobody is told"); }
        if (t == 15) { s.nodes[1].alive = true; log(t, "world", "k2 is back"); }
        // Every loop runs every tick and reads the store; none of them calls another.
        nodeAgents(s, t);
        nodeController(s, t);
        replicaController(s, t);
        scheduler(s, t);
    }
    std::cout << "final pods:";
    for (const Pod& p : s.pods) {
        if (p.phase != "Deleted") {
            std::cout << ' ' << p.name << '@' << (p.node.empty() ? "-" : p.node) << '(' << p.phase
                      << ')';
        }
    }
    std::cout << '\n';
    return 0;
}
