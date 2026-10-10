// inventory.cpp - read a cluster's node table and decide, for each request,
// whether it can run now, later, or never (F5-22, DS303).
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

struct Node
{
    std::string name;
    int cpus = 0;
    int memGiB = 0;
    int gpus = 0;
    std::string state;  // idle, busy or down
};

struct Request
{
    std::string id;
    int nodes = 0;          // how many nodes the job needs at the same time
    int cpusPerNode = 0;
    int memGiBPerNode = 0;
    int gpusPerNode = 0;
};

bool nodeCanHold(const Node& n, const Request& r)
{
    return n.cpus >= r.cpusPerNode && n.memGiB >= r.memGiBPerNode && n.gpus >= r.gpusPerNode;
}

int main()
{
    std::vector<Node> nodes;
    std::vector<Request> requests;
    std::string line;
    bool readingRequests = false;
    while (std::getline(std::cin, line)) {
        if (line.empty() || line[0] == '#') {
            continue;
        }
        if (line == "requests") {
            readingRequests = true;
            continue;
        }
        std::istringstream in(line);
        if (readingRequests) {
            Request r;
            in >> r.id >> r.nodes >> r.cpusPerNode >> r.memGiBPerNode >> r.gpusPerNode;
            requests.push_back(r);
        } else {
            Node n;
            in >> n.name >> n.cpus >> n.memGiB >> n.gpus >> n.state;
            nodes.push_back(n);
        }
    }

    int up = 0, idle = 0, cpus = 0, mem = 0, gpus = 0;
    for (const Node& n : nodes) {
        if (n.state == "down") {
            continue;
        }
        ++up;
        cpus += n.cpus;
        mem += n.memGiB;
        gpus += n.gpus;
        if (n.state == "idle") {
            ++idle;
        }
    }
    std::cout << "nodes: " << nodes.size() << " listed, " << up << " up, " << idle << " idle\n";
    std::cout << "capacity of the up nodes: " << cpus << " CPUs, " << mem << " GiB, " << gpus
              << " GPUs\n\n";

    for (const Request& r : requests) {
        int holdNow = 0, holdLater = 0;
        for (const Node& n : nodes) {
            if (n.state == "down" || !nodeCanHold(n, r)) {
                continue;
            }
            ++holdLater;                 // fits once the node is free
            if (n.state == "idle") {
                ++holdNow;               // fits right now
            }
        }
        std::string verdict = "NEVER: no set of up nodes can hold it";
        if (holdNow >= r.nodes) {
            verdict = "runs now";
        } else if (holdLater >= r.nodes) {
            verdict = "waits for busy nodes to finish";
        }
        std::cout << r.id << ": " << r.nodes << " node(s) x (" << r.cpusPerNode << " CPUs, "
                  << r.memGiBPerNode << " GiB, " << r.gpusPerNode << " GPUs) -> nodes able now "
                  << holdNow << ", ever " << holdLater << " -> " << verdict << '\n';
    }
    return 0;
}
