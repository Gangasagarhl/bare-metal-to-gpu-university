// capacity.cpp - F12-15 Listing 1: a capacity plan built from an SLO, a load test and a forecast.
// Step 1  load test of one node (node_model.hpp): the highest offered rate whose p99 meets the
//         SLO is the node's SAFE rate (not its CPU capacity).
// Step 2  demand: the busiest hour of an exercise day profile, times a peak-minute factor,
//         times compound growth over the planning horizon.
// Step 3  nodes = ceil(peak demand / safe rate) + 1 spare, so the SLO still holds with one
//         node down (failure or maintenance).
// Step 4  the same arithmetic done the naive way (average load, CPU utilisation target),
//         for comparison. All inputs are exercise data, printed with the plan.
#include "node_model.hpp"

#include <cmath>
#include <cstdio>
#include <vector>

int main()
{
    double const slo_p99_ms = 25.0;
    std::printf("== Step 1: load test of one node (simulated, 120 s per row), SLO p99 <= %.0f ms\n",
                slo_p99_ms);
    std::printf("%8s %8s %9s %9s  %s\n", "req/s", "CPU %", "p50 ms", "p99 ms", "meets SLO");
    double safe = 0.0;
    for (double rate = 400.0; rate <= 950.0; rate += 50.0) {
        auto const lat = node::simulate(rate, 120.0, 1);
        double const p99 = node::percentile(lat, 99.0);
        bool const ok = p99 <= slo_p99_ms;
        if (ok) {
            safe = rate;
        }
        std::printf("%8.0f %7.0f%% %9.2f %9.2f  %s\n", rate,
                    100.0 * rate * node::kServiceMs / 1000.0 / node::kWorkers,
                    node::percentile(lat, 50.0), p99, ok ? "yes" : "NO");
    }
    double const cpu_capacity = 1000.0 * node::kWorkers / node::kServiceMs;
    std::printf(
        "CPU capacity %.0f req/s; safe rate (highest row meeting the SLO) %.0f req/s = %.0f %% "
        "CPU\n\n",
        cpu_capacity, safe, 100.0 * safe / cpu_capacity);

    // Exercise day profile: average req/s in each hour 00..23 (whole service, today).
    std::vector<double> const hourly = {420,  300,  240,  220,  230,  310,  620,  1250,
                                        1900, 2100, 2150, 2300, 2600, 2450, 2200, 2150,
                                        2250, 2500, 2900, 3150, 3000, 2400, 1500, 800};
    double sum = 0.0;
    double busiest = 0.0;
    for (double h : hourly) {
        sum += h;
        busiest = std::max(busiest, h);
    }
    double const average = sum / static_cast<double>(hourly.size());
    double const peak_minute_factor = 1.25;  // busiest minute / busiest hour, from telemetry
    double const growth_per_month = 0.05;
    int const months = 9;
    double const growth = std::pow(1.0 + growth_per_month, months);
    double const demand = busiest * peak_minute_factor * growth;
    std::printf("== Step 2: demand\n");
    std::printf("daily average %.0f req/s; busiest hour %.0f req/s (peak-to-average %.2f)\n",
                average, busiest, busiest / average);
    std::printf("busiest minute = busiest hour x %.2f = %.0f req/s\n", peak_minute_factor,
                busiest * peak_minute_factor);
    std::printf("growth %.0f %% per month for %d months: x %.3f -> planned peak %.0f req/s\n\n",
                100.0 * growth_per_month, months, growth, demand);

    int const needed = static_cast<int>(std::ceil(demand / safe));
    std::printf("== Step 3: nodes\n");
    std::printf("ceil(%.0f / %.0f) = %d nodes carry the planned peak within the SLO\n", demand,
                safe, needed);
    std::printf("+1 spare so one node can fail or be updated at peak: %d nodes\n", needed + 1);
    std::printf("check: with one node down, each of %d nodes takes %.0f req/s (safe rate %.0f)\n\n",
                needed, demand / needed, safe);

    double const naive_target = 0.70;
    int const naive = static_cast<int>(std::ceil(average * growth / (cpu_capacity * naive_target)));
    std::printf("== Step 4: the naive plan, for comparison\n");
    std::printf("average x growth / (CPU capacity x %.0f %%) = %.0f / %.0f -> %d nodes, no spare\n",
                100.0 * naive_target, average * growth, cpu_capacity * naive_target, naive);
    std::printf(
        "at the planned peak those %d nodes would each take %.0f req/s (CPU capacity %.0f)\n",
        naive, demand / naive, cpu_capacity);
    return 0;
}
