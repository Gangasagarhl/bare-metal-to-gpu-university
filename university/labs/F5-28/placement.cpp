// placement.cpp - the worked example of F5-28: how far, on average, a thread's memory accesses
// travel when its pages are spread over NUMA nodes. Input: the node distance table (the SLIT
// values printed by numa.cc in QEMU) and placements. SLIT distances are RELATIVE numbers
// (10 means "local"); this program does not turn them into nanoseconds.
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

struct Placement
{
    std::string name;
    int cpu_node;                    // node of the CPU the thread runs on
    std::vector<double> page_share;  // share of the thread's accesses that go to each node
};

int main()
{
    int n = 0;
    std::cin >> n;
    std::vector<std::vector<int>> dist(static_cast<std::size_t>(n), std::vector<int>(static_cast<std::size_t>(n)));
    for (auto& row : dist) {
        for (int& d : row) {
            std::cin >> d;
        }
    }
    std::vector<Placement> cases;
    std::string name;
    while (std::cin >> name) {
        Placement p{name, 0, std::vector<double>(static_cast<std::size_t>(n))};
        std::cin >> p.cpu_node;
        for (double& s : p.page_share) {
            std::cin >> s;
        }
        cases.push_back(p);
    }
    std::cout << std::fixed << std::setprecision(2);
    for (const Placement& p : cases) {
        double weighted = 0.0;
        double total = 0.0;
        for (int node = 0; node < n; ++node) {
            const double s = p.page_share[static_cast<std::size_t>(node)];
            weighted += s * dist[static_cast<std::size_t>(p.cpu_node)][static_cast<std::size_t>(node)];
            total += s;
        }
        const double local = p.page_share[static_cast<std::size_t>(p.cpu_node)] / total;
        std::cout << std::left << std::setw(22) << p.name
                  << " CPU on node " << p.cpu_node
                  << "  local share " << std::setw(5) << local * 100.0 << " %"
                  << "  mean distance " << std::setw(6) << weighted / total
                  << "  relative to all-local " << weighted / total / 10.0 << '\n';
    }
    return 0;
}
