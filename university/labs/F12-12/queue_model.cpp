// queue_model.cpp - F12-12 Listing 1: predict with a queueing model, then check by simulation.
// One server, first come first served, Poisson arrivals (exponential gaps), mean service S.
// Predictions (formulas derived in the chapter, Layer 2 and 3):
//   M/M/1 (exponential service):  mean response R = S / (1 - U),   p99 = S * ln(100) / (1 - U)
//   M/D/1 (constant service):     mean response R = S + U * S / (2 * (1 - U))
// where U = arrival rate * S is the utilisation. The simulation uses Lindley's recursion:
// a request starts at max(its arrival, the previous request's finish).
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <vector>

struct Rng
{
    std::uint64_t s;
    std::uint64_t next()
    {
        std::uint64_t z = (s += 0x9E3779B97F4A7C15ULL);
        z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
        z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
        return z ^ (z >> 31);
    }
    double uniform() { return static_cast<double>(next() >> 11) * 0x1.0p-53; }
    double exponential(double mean) { return -mean * std::log(1.0 - uniform()); }
};

struct Result
{
    double mean;
    double p99;
};

Result simulate(double u, bool constant_service, std::uint64_t seed)
{
    double const s = 1.0;  // mean service time: 1 ms
    std::size_t const n = 1000000;
    Rng rng{seed};
    std::vector<double> r(n);
    double arrival = 0.0;
    double free_at = 0.0;
    double sum = 0.0;
    for (std::size_t i = 0; i < n; ++i) {
        arrival += rng.exponential(s / u);  // mean gap = 1 / rate = S / U
        double const start = std::max(arrival, free_at);
        free_at = start + (constant_service ? s : rng.exponential(s));
        r[i] = free_at - arrival;
        sum += r[i];
    }
    auto const k = static_cast<long>(std::ceil(0.99 * static_cast<double>(n))) - 1;
    std::nth_element(r.begin(), r.begin() + k, r.end());
    return {sum / static_cast<double>(n), r[static_cast<std::size_t>(k)]};
}

int main()
{
    std::printf("service time S = 1 ms; 1,000,000 simulated requests per row; times in ms\n\n");
    std::printf(
        "            ---------- M/M/1 (exponential service) ----------   -- M/D/1 (constant) --\n");
    std::printf(
        "   U    mean model  mean sim   p99 model   p99 sim   err p99   mean model  mean sim\n");
    for (double u : {0.10, 0.30, 0.50, 0.70, 0.80, 0.90, 0.95}) {
        Result const mm1 = simulate(u, false, 12);
        Result const md1 = simulate(u, true, 12);
        double const mean_mm1 = 1.0 / (1.0 - u);
        double const p99_mm1 = std::log(100.0) / (1.0 - u);
        double const mean_md1 = 1.0 + u / (2.0 * (1.0 - u));
        std::printf("%5.2f %11.2f %9.2f %11.2f %9.2f %8.1f%% %11.2f %9.2f\n", u, mean_mm1, mm1.mean,
                    p99_mm1, mm1.p99, 100.0 * (mm1.p99 - p99_mm1) / p99_mm1, mean_md1, md1.mean);
    }
    std::printf("\nslope check: going from U = 0.80 to 0.90 doubles 1/(1-U) (5 -> 10);\n");
    std::printf("going from U = 0.90 to 0.95 doubles it again (10 -> 20).\n");
    return 0;
}
