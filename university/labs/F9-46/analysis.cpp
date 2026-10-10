// analysis.cpp - F9-46 Listing 2: schedulability tests on paper, as a program.
//  * fixed priorities: Liu-Layland bound, hyperbolic bound, and response-time analysis (RTA)
//    with a blocking term B and release jitter J;
//  * EDF: the utilisation test (deadlines = periods) and the processor-demand test
//    (deadlines shorter than periods).
// All times are integers (microseconds or abstract units), so ceilings are exact.
#include <cmath>
#include <cstdio>
#include <numeric>
#include <vector>

struct Task {
    const char* name;
    long T;   // period
    long C;   // worst-case execution time
    long D;   // relative deadline
    long B;   // worst blocking by lower-priority tasks (shared locks), fixed priorities only
    long J;   // release jitter: how late, at most, the task is released after its due time
};

long ceilDiv(long a, long b) { return (a + b - 1) / b; }

void fixedPriority(const char* title, const std::vector<Task>& ts)   // ts in priority order
{
    double u = 0, hyper = 1;
    for (const Task& t : ts) {
        const double ui = static_cast<double>(t.C) / static_cast<double>(t.T);
        u += ui;
        hyper *= ui + 1.0;
    }
    const double n = static_cast<double>(ts.size());
    const double ll = n * (std::pow(2.0, 1.0 / n) - 1.0);
    std::printf("%s\n", title);
    std::printf("  U = %.4f; Liu-Layland bound for n=%zu: %.4f -> %s\n", u, ts.size(), ll,
                u <= ll ? "schedulable (sufficient test)" : "test inconclusive");
    std::printf("  hyperbolic bound: product of (Ui+1) = %.4f -> %s\n", hyper,
                hyper <= 2.0 ? "schedulable (sufficient test)" : "test inconclusive");
    for (std::size_t i = 0; i < ts.size(); ++i) {
        const Task& t = ts[i];
        long w = t.C + t.B;
        long prev = -1;
        int iterations = 0;
        while (w != prev && w + t.J <= t.D) {     // stop at the fixed point or past the deadline
            prev = w;
            long next = t.C + t.B;
            for (std::size_t j = 0; j < i; ++j) {
                next += ceilDiv(w + ts[j].J, ts[j].T) * ts[j].C;
            }
            w = next;
            ++iterations;
        }
        const long r = w + t.J;
        std::printf("  %-10s T=%6ld C=%6ld D=%6ld B=%4ld J=%4ld  R=%6ld  %s (%d iterations)\n",
                    t.name, t.T, t.C, t.D, t.B, t.J, r,
                    r <= t.D ? "meets D" : "MISSES D", iterations);
    }
}

void edf(const char* title, const std::vector<Task>& ts)
{
    double u = 0;
    long h = 1, maxD = 0;
    bool implicit = true;
    for (const Task& t : ts) {
        u += static_cast<double>(t.C) / static_cast<double>(t.T);
        h = std::lcm(h, t.T);
        maxD = t.D > maxD ? t.D : maxD;
        implicit = implicit && t.D == t.T;
    }
    std::printf("%s\n  U = %.4f, hyperperiod %ld\n", title, u, h);
    if (implicit) {
        std::printf("  deadlines = periods: EDF schedulable if and only if U <= 1 -> %s\n",
                    u <= 1.0 ? "schedulable" : "NOT schedulable");
        return;
    }
    // processor demand: in any interval of length L, the work that must be done
    // dbf(L) = sum over tasks of max(0, floor((L - D)/T) + 1) * C must not exceed L.
    bool ok = u <= 1.0;
    int shown = 0;
    // for tasks all released together at time 0 it is enough to check L up to H + max D
    for (long L = 1; L <= h + maxD && ok; ++L) {
        long dbf = 0;
        bool isDeadline = false;
        for (const Task& t : ts) {
            if (L >= t.D) {
                dbf += ((L - t.D) / t.T + 1) * t.C;
                isDeadline = isDeadline || (L - t.D) % t.T == 0;
            }
        }
        if (isDeadline && shown < 8) {
            std::printf("  L=%3ld  dbf=%3ld  %s\n", L, dbf, dbf <= L ? "ok" : "DEMAND EXCEEDS TIME");
            ++shown;
        }
        if (dbf > L) {
            if (shown >= 8) { std::printf("  L=%3ld  dbf=%3ld  DEMAND EXCEEDS TIME\n", L, dbf); }
            ok = false;
        }
    }
    std::printf("  processor-demand test for every L up to H + max D: %s\n",
                ok ? "schedulable" : "NOT schedulable");
}

int main()
{
    // the set simulated in Listing 1 (abstract units): RM fails although U = 1
    fixedPriority("set 1 under rate-monotonic priorities (units)",
                  {{"A", 4, 2, 4, 0, 0}, {"B", 6, 3, 6, 0, 0}});
    edf("set 1 under EDF (units)", {{"A", 4, 2, 4, 0, 0}, {"B", 6, 3, 6, 0, 0}});

    // a robot controller in microseconds; J for control is the timer lateness budget,
    // B is the longest critical section of a lower-priority task on a shared lock.
    fixedPriority("\nrobot set, rate-monotonic (microseconds)",
                  {{"control", 1000, 200, 1000, 150, 100},
                   {"estimator", 5000, 1000, 5000, 150, 0},
                   {"planner", 50000, 12000, 50000, 0, 0}});
    fixedPriority("\nrobot set, planner grows to 24 ms",
                  {{"control", 1000, 200, 1000, 150, 100},
                   {"estimator", 5000, 1000, 5000, 150, 0},
                   {"planner", 50000, 24000, 50000, 0, 0}});

    // EDF with deadlines shorter than periods (units)
    edf("\nconstrained set X under EDF (units)", {{"P", 6, 2, 4, 0, 0}, {"Q", 8, 3, 6, 0, 0},
                                                  {"R", 12, 2, 12, 0, 0}});
    edf("\nconstrained set Y under EDF (units)", {{"P", 6, 2, 3, 0, 0}, {"Q", 8, 3, 4, 0, 0},
                                                  {"R", 12, 2, 12, 0, 0}});
    return 0;
}
