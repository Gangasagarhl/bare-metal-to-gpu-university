// outage.cpp - F12-15 forensic evidence generator: "enough servers, on paper".
// Exercise simulation (node_model.hpp, fixed seeds), not a measurement. Prints the capacity
// plan a team approved, the launch-day traffic and the per-node latency the gateway saw.
// Each hourly row simulates 10 minutes of that hour's traffic on one node, starting empty.
#include "node_model.hpp"

#include <cstdio>
#include <vector>

int main()
{
    std::printf("== evidence 1: capacity plan approved on 2026-06-02 (excerpt)\n");
    std::printf("  expected launch traffic: 1,900 req/s (pilot average, scaled to all users)\n");
    std::printf("  node capacity: 4 workers / 4 ms per request = 1,000 req/s\n");
    std::printf(
        "  target CPU utilisation 70 %% -> 1,900 / 700 = 2.7 -> 3 nodes; deploy 4 for safety\n");
    std::printf("  SLO: p99 latency at the gateway <= 25 ms\n\n");

    struct Hour
    {
        int hour;
        double total;
        int nodes;
    };
    // Launch day. At 20:00 one node left service for a kernel update planned months earlier.
    std::vector<Hour> const day = {{9, 1500, 4},  {11, 2100, 4}, {13, 2500, 4}, {15, 2300, 4},
                                   {17, 2700, 4}, {18, 3000, 4}, {19, 3300, 4}, {20, 3600, 3},
                                   {21, 3100, 3}, {22, 2000, 4}};
    double total_requests = 0.0;
    double weighted = 0.0;
    std::printf(
        "== evidence 2: launch day, gateway metrics per node (10-minute sample of each hour)\n");
    std::printf("%5s %10s %6s %13s %8s %9s %10s\n", "hour", "total r/s", "nodes", "per node r/s",
                "CPU %", "p50 ms", "p99 ms");
    for (Hour const& h : day) {
        double const per_node = h.total / h.nodes;
        auto const lat = node::simulate(per_node, 600.0, static_cast<std::uint64_t>(h.hour));
        std::printf("%3d:00 %10.0f %6d %13.0f %7.0f%% %9.1f %10.1f\n", h.hour, h.total, h.nodes,
                    per_node, std::min(100.0, per_node * node::kServiceMs / 10.0 / node::kWorkers),
                    node::percentile(lat, 50.0), node::percentile(lat, 99.0));
        total_requests += h.total;
        weighted += 1.0;
    }
    std::printf("average of the rows above: %.0f req/s\n\n", total_requests / weighted);
    std::printf("== evidence 3: change calendar\n");
    std::printf(
        "  2026-03-14 (planned): weekly maintenance window 20:00-22:00, one node at a time\n");
    std::printf("  launch day 20:00: node-3 drained for kernel update; back in service 22:00\n");
    return 0;
}
