// F8-04 Listing 1: a topology model of TN-1, the university's invented teaching node.
// TN-1: two CPU sockets joined by a CPU-to-CPU link; each socket's root complex has two PCIe
// switches; each switch has two GPUs; GPUs 0-1, 2-3, 4-5, 6-7 also have a direct GPU bridge.
// ALL bandwidths are invented teaching values (GB/s per direction), not any product's numbers.
// The program prints (1) the path class and bottleneck bandwidth of every GPU pair and
// (2) for each job in topo_sim.in, every ring order's bandwidth when all hops send at once.
#include <algorithm>
#include <cstdio>
#include <iostream>
#include <map>
#include <queue>
#include <sstream>
#include <string>
#include <vector>

struct Edge
{
    int a;
    int b;
    double gbps;        // per direction
    std::string kind;   // "pcie", "cpu-link", "bridge"
};

struct Node
{
    std::vector<std::string> names;
    std::vector<Edge> edges;
    int add(const std::string& n) { names.push_back(n); return static_cast<int>(names.size()) - 1; }
};

Node buildTN1()
{
    Node t;
    const int cpu0 = t.add("RC0");                   // CPU 0 and its PCIe root complex
    const int cpu1 = t.add("RC1");
    t.edges.push_back({cpu0, cpu1, 12.5, "cpu-link"});
    for (int s = 0; s < 4; ++s) {
        const int sw = t.add("SW" + std::to_string(s));
        t.edges.push_back({s < 2 ? cpu0 : cpu1, sw, 25.0, "pcie"});
    }
    for (int g = 0; g < 8; ++g) {
        const int gpu = t.add("GPU" + std::to_string(g));
        t.edges.push_back({2 + g / 2, gpu, 25.0, "pcie"});   // switch g/2 (node ids 2..5)
    }
    for (int g = 0; g < 8; g += 2) {
        t.edges.push_back({6 + g, 7 + g, 50.0, "bridge"});    // GPU node ids are 6..13
    }
    return t;
}

int gpuNode(int g) { return 6 + g; }

// shortest path (fewest links) as a list of edge indices with direction (+1 a->b, -1 b->a)
std::vector<std::pair<int, int>> route(const Node& t, int from, int to)
{
    const int n = static_cast<int>(t.names.size());
    std::vector<int> prevEdge(static_cast<std::size_t>(n), -1), prevNode(static_cast<std::size_t>(n), -1);
    std::vector<bool> seen(static_cast<std::size_t>(n), false);
    std::queue<int> q;
    q.push(from);
    seen[static_cast<std::size_t>(from)] = true;
    while (!q.empty()) {
        const int u = q.front();
        q.pop();
        for (int e = 0; e < static_cast<int>(t.edges.size()); ++e) {
            const Edge& ed = t.edges[static_cast<std::size_t>(e)];
            int v = -1;
            if (ed.a == u) { v = ed.b; }
            if (ed.b == u) { v = ed.a; }
            if (v >= 0 && !seen[static_cast<std::size_t>(v)]) {
                seen[static_cast<std::size_t>(v)] = true;
                prevEdge[static_cast<std::size_t>(v)] = e;
                prevNode[static_cast<std::size_t>(v)] = u;
                q.push(v);
            }
        }
    }
    std::vector<std::pair<int, int>> path;
    for (int v = to; v != from; v = prevNode[static_cast<std::size_t>(v)]) {
        const int e = prevEdge[static_cast<std::size_t>(v)];
        path.push_back({e, t.edges[static_cast<std::size_t>(e)].b == v ? +1 : -1});
    }
    std::reverse(path.begin(), path.end());
    return path;
}

// the university's own path classes (not any vendor tool's legend)
std::string pathClass(const Node& t, const std::vector<std::pair<int, int>>& p)
{
    bool cpuLink = false, viaRoot = false;
    for (const auto& [e, dir] : p) {
        const Edge& ed = t.edges[static_cast<std::size_t>(e)];
        if (ed.kind == "bridge" && p.size() == 1) { return "BR"; }
        if (ed.kind == "cpu-link") { cpuLink = true; }
        if (ed.a <= 1 || ed.b <= 1) { viaRoot = true; }
        (void)dir;
    }
    return cpuLink ? "SOCK" : (viaRoot ? "RC" : "SW");
}

std::string describe(const Node& t, int from, const std::vector<std::pair<int, int>>& p)
{
    std::string s = t.names[static_cast<std::size_t>(from)];
    for (const auto& [e, dir] : p) {
        const Edge& ed = t.edges[static_cast<std::size_t>(e)];
        s += ">" + t.names[static_cast<std::size_t>(dir > 0 ? ed.b : ed.a)];
    }
    return s;
}

// all hops of a ring send at once; a directed link shared by k hops gives each 1/k of it
double ringGbps(const Node& t, const std::vector<int>& order, std::string* worstHop)
{
    std::map<std::pair<int, int>, int> load;             // (edge, direction) -> hops using it
    std::vector<std::vector<std::pair<int, int>>> paths;
    for (std::size_t i = 0; i < order.size(); ++i) {
        const int a = gpuNode(order[i]);
        const int b = gpuNode(order[(i + 1) % order.size()]);
        paths.push_back(route(t, a, b));
        for (const auto& ed : paths.back()) {
            ++load[ed];
        }
    }
    double ring = 1e30;
    for (std::size_t i = 0; i < paths.size(); ++i) {
        double hop = 1e30;
        for (const auto& ed : paths[i]) {
            hop = std::min(hop, t.edges[static_cast<std::size_t>(ed.first)].gbps / load[ed]);
        }
        if (hop < ring) {
            ring = hop;
            if (worstHop) {
                *worstHop = describe(t, gpuNode(order[i]), paths[i]) + " (" + pathClass(t, paths[i]) + ")";
            }
        }
    }
    return ring;
}

std::string orderText(const std::vector<int>& o)
{
    std::string s;
    for (int g : o) { s += std::to_string(g) + "->"; }
    return s + std::to_string(o[0]);
}

int main()
{
    const Node t = buildTN1();
    std::printf("TN-1 (invented teaching node): path class / bottleneck GB/s for every GPU pair\n");
    std::printf("classes: BR = direct GPU bridge, SW = same PCIe switch, RC = through one root complex,\n"
                "         SOCK = through both root complexes and the CPU-to-CPU link\n      ");
    for (int b = 0; b < 8; ++b) { std::printf("   GPU%d  ", b); }
    std::printf("\n");
    for (int a = 0; a < 8; ++a) {
        std::printf("GPU%d  ", a);
        for (int b = 0; b < 8; ++b) {
            if (a == b) { std::printf("    X    "); continue; }
            const auto p = route(t, gpuNode(a), gpuNode(b));
            double bw = 1e30;
            for (const auto& ed : p) { bw = std::min(bw, t.edges[static_cast<std::size_t>(ed.first)].gbps); }
            std::printf(" %4s %4.1f", pathClass(t, p).c_str(), bw);
        }
        std::printf("\n");
    }
    std::string line;
    while (std::getline(std::cin, line)) {               // one job per line: GPU numbers
        std::istringstream in(line);
        std::vector<int> gpus;
        for (int g; in >> g;) { gpus.push_back(g); }
        if (gpus.size() < 2) { continue; }
        std::sort(gpus.begin() + 1, gpus.end());
        double best = -1, worst = 1e30;
        std::vector<int> bestO, worstO;
        std::string bestHop, worstHop;
        int orders = 0;
        do {                                             // every ring order starting at gpus[0]
            std::string hop;
            const double bw = ringGbps(t, gpus, &hop);
            ++orders;
            if (bw > best) { best = bw; bestO = gpus; bestHop = hop; }
            if (bw < worst) { worst = bw; worstO = gpus; worstHop = hop; }
        } while (std::next_permutation(gpus.begin() + 1, gpus.end()));
        std::printf("\njob on GPUs %s: %d ring orders tried\n", line.c_str(), orders);
        std::printf("  best ring  %-26s %5.2f GB/s, slowest hop %s\n", orderText(bestO).c_str(), best, bestHop.c_str());
        std::printf("  worst ring %-26s %5.2f GB/s, slowest hop %s\n", orderText(worstO).c_str(), worst, worstHop.c_str());
    }
    return 0;
}
