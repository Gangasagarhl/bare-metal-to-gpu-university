// availability.cpp - how the number of machines changes the chance that a service is up.
// Each machine is up with probability a, independently of the others (an assumption
// this program states, and that the forensic lab breaks). Three ways to use n machines:
//   all needed  : the service needs every machine (data split over n machines, no copies)
//   any one     : one machine is enough (n full copies)
//   majority    : more than half must be up (the rule of the consensus course, DS302)
// The closed formulas are checked against a simulation with a fixed-seed generator.
#include <cstdint>
#include <cstdio>
#include <vector>

// splitmix64: a small pseudo-random generator defined entirely by this arithmetic,
// so every run (and every machine) produces the same sequence.
struct Rng
{
    std::uint64_t state;
    std::uint64_t next()
    {
        std::uint64_t z = (state += 0x9E3779B97F4A7C15ULL);
        z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
        z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
        return z ^ (z >> 31);
    }
    double uniform() { return static_cast<double>(next() >> 11) * 0x1.0p-53; }  // [0, 1)
};

double power(double x, int n)
{
    double r = 1.0;
    for (int i = 0; i < n; ++i) {
        r *= x;
    }
    return r;
}

// P(at least k of n machines are up) = sum over j >= k of C(n, j) a^j (1 - a)^(n - j)
double atLeast(int k, int n, double a)
{
    double sum = 0.0;
    double choose = 1.0;                       // C(n, 0)
    for (int j = 0; j <= n; ++j) {
        if (j >= k) {
            sum += choose * power(a, j) * power(1.0 - a, n - j);
        }
        choose = choose * (n - j) / (j + 1);   // C(n, j + 1) from C(n, j)
    }
    return sum;
}

int main()
{
    const double a = 0.99;                     // an exercise value, not a measured machine
    std::printf("each machine up with probability a = %.2f, failures independent\n\n", a);
    std::printf("%4s  %12s  %12s  %12s\n", "n", "all needed", "any one", "majority");
    for (int n : {1, 2, 3, 5, 10, 100}) {
        std::printf("%4d  %12.6f  %12.10f  %12.10f\n", n, power(a, n),
                    1.0 - power(1.0 - a, n), atLeast(n / 2 + 1, n, a));
    }

    // Simulation: many independent "days"; on each day every machine is up with probability a.
    Rng rng{2026};
    const int trials = 1000000;
    std::printf("\nsimulation, %d trials per row (fixed seed)\n", trials);
    std::printf("%4s  %12s  %12s  %12s\n", "n", "all needed", "any one", "majority");
    for (int n : {1, 3, 5}) {
        int all = 0, any = 0, maj = 0;
        for (int t = 0; t < trials; ++t) {
            int up = 0;
            for (int i = 0; i < n; ++i) {
                up += rng.uniform() < a ? 1 : 0;
            }
            all += up == n ? 1 : 0;
            any += up >= 1 ? 1 : 0;
            maj += up > n / 2 ? 1 : 0;
        }
        std::printf("%4d  %12.6f  %12.6f  %12.6f\n", n, 1.0 * all / trials,
                    1.0 * any / trials, 1.0 * maj / trials);
    }

    // The same three copies when a shared part (one power feed) fails with probability q
    // and takes all of them down at once: copies cannot beat 1 - q.
    std::printf("\nthree copies on one shared power feed (any one copy is enough)\n");
    std::printf("%10s  %14s  %14s\n", "q (feed)", "formula", "simulation");
    for (double q : {0.0, 0.001, 0.01}) {
        int ok = 0;
        for (int t = 0; t < trials; ++t) {
            const bool feedUp = rng.uniform() >= q;
            int up = 0;
            for (int i = 0; i < 3; ++i) {
                up += rng.uniform() < a ? 1 : 0;
            }
            ok += (feedUp && up >= 1) ? 1 : 0;
        }
        std::printf("%10.3f  %14.8f  %14.6f\n", q, (1.0 - q) * (1.0 - power(1.0 - a, 3)),
                    1.0 * ok / trials);
    }
    return 0;
}
