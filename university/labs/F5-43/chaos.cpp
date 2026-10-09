// chaos.cpp - fault-injection experiments on a model of a three-replica service (F5-43, Listing 1).
// Clients send requests through a load balancer to replicas r0, r1, r2; each replica can serve
// 700 requests/s. A replica's response time is a fixed delay plus an M/M/1 queue: exponential
// with mean 1/(capacity - load). A client gives up after 250 ms and may retry; the replica still
// does the work of a request whose client gave up. Every experiment states the steady-state
// hypothesis "success >= 99.5 % in every 10 s window", injects one fault into r2 at t = 60 s,
// and is aborted (fault removed) if success falls below 95 % in a 10 s window. A fluid model in
// 100 ms steps with exercise values: it shows mechanisms, not measurements of any system.
// Printed: the 10 s windows ending at 60, 70, 80, 90 and 180 s (and any window that aborts).
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

constexpr double kDt = 0.1;           // s per step
constexpr double kCapacity = 700.0;   // requests per s, per replica
constexpr double kTimeout = 0.250;    // s
constexpr double kBase = 0.002;       // s outside the queue (network, parsing)
constexpr int kR = 3;

using PerReplica = std::array<double, kR>;

struct Experiment
{
    std::string name;
    double offered;              // new requests per s
    bool kill = false;           // r2 refuses connections from t = 60 s
    double extraDelay = 0.0;     // s added to r2's responses from t = 60 s
    int maxAttempts = 1;         // 1 = no retry
    bool otherReplica = false;   // a retry avoids the replica that just failed
    double retryBudget = 1e9;    // retries per step at most this share of new requests
};

// share of responses slower than the client's timeout, for a replica with this load and delay
double lateShare(double load, double delay)
{
    if (load >= kCapacity) {
        return 1.0;   // overloaded: in this model the queue grows without bound
    }
    const double spare = kTimeout - delay;
    return spare <= 0 ? 1.0 : std::exp(-(kCapacity - load) * spare);
}

void run(const Experiment& e)
{
    std::printf("== %s\n", e.name.c_str());
    std::printf("   window  attempts/s  amplification  success %%  r0 load  r1 load  r2 load  (%% of capacity)\n");
    std::vector<PerReplica> retry(4, PerReplica{});   // retries due next step, by attempt number
    double winNew = 0;
    double winFail = 0;
    double winAtt = 0;
    PerReplica winLoad{};
    bool held = true;
    for (int s = 0; s < static_cast<int>(180.0 / kDt); ++s) {
        const double t = s * kDt;
        const bool fault = t >= 60.0;
        const bool r2Out = e.kill && t >= 63.0;   // health check removes a dead r2 after 3 s
        const int inRotation = r2Out ? 2 : 3;
        // attempts per replica this step: new requests spread evenly, plus due retries
        std::vector<PerReplica> att(4, PerReplica{});
        for (int r = 0; r < inRotation; ++r) {
            att[1][static_cast<std::size_t>(r)] = e.offered * kDt / inRotation;
        }
        att[2] = retry[2];
        att[3] = retry[3];
        retry[2] = retry[3] = PerReplica{};
        PerReplica load{};
        PerReplica late{};
        for (int r = 0; r < kR; ++r) {
            const auto ru = static_cast<std::size_t>(r);
            const double n = att[1][ru] + att[2][ru] + att[3][ru];
            const bool refused = r == 2 && fault && e.kill;
            load[ru] = refused ? 0.0 : n / kDt;   // a refused connection costs the replica nothing
            const double delay = kBase + (r == 2 && fault ? e.extraDelay : 0.0);
            late[ru] = refused ? 1.0 : lateShare(load[ru], delay);
        }
        double budget = e.retryBudget * e.offered * kDt;
        double failNow = 0;
        double attNow = 0;
        for (int k = 1; k <= 3; ++k) {
            for (int r = 0; r < kR; ++r) {
                const auto ru = static_cast<std::size_t>(r);
                const double failed = att[static_cast<std::size_t>(k)][ru] * late[ru];
                attNow += att[static_cast<std::size_t>(k)][ru];
                if (k >= e.maxAttempts) {
                    failNow += failed;
                    continue;
                }
                const double again = std::min(failed, budget);
                budget -= again;
                failNow += failed - again;
                // where the retries go: every replica in rotation, or every other one
                std::vector<int> to;
                for (int q = 0; q < inRotation; ++q) {
                    if (!(e.otherReplica && q == r)) {
                        to.push_back(q);
                    }
                }
                for (int q : to) {
                    retry[static_cast<std::size_t>(k + 1)][static_cast<std::size_t>(q)] += again / static_cast<double>(to.size());
                }
            }
        }
        winNew += e.offered * kDt;
        winFail += failNow;
        winAtt += attNow;
        for (int r = 0; r < kR; ++r) {
            winLoad[static_cast<std::size_t>(r)] += load[static_cast<std::size_t>(r)] / kCapacity;
        }
        if ((s + 1) % 100 == 0) {
            const double success = 100.0 * (1.0 - winFail / winNew);
            const int end = s / 100 + 1;   // window number: it ends at 10 * end seconds
            if (end == 6 || (end >= 7 && end <= 9) || end == 18 || success < 95.0) {
                std::printf("   %4.0f s  %9.0f  %13.2f  %9.2f  %6.0f%%  %6.0f%%  %6.0f%%\n", t + kDt, winAtt / 10.0,
                            winAtt / winNew, success, winLoad[0], winLoad[1], winLoad[2]);
            }
            held = held && success >= 99.5;
            if (success < 95.0) {
                std::printf("   ABORT at t = %.0f s: success %.2f %% < 95 %% in the last 10 s; fault removed\n",
                            t + kDt, success);
                break;
            }
            winNew = winFail = winAtt = 0;
            winLoad = PerReplica{};
        }
    }
    std::printf("   steady-state hypothesis: %s\n\n", held ? "HELD" : "REJECTED");
}

int main()
{
    const std::vector<Experiment> ex = {
        {"E0 baseline, 900 requests/s, no fault", 900, false, 0.0, 1, false, 1e9},
        {"E1 900/s: r2 killed; one retry, to another replica", 900, true, 0.0, 2, true, 1e9},
        {"E2 900/s: r2 300 ms slow; up to 3 attempts, retries to any replica", 900, false, 0.300, 3, false, 1e9},
        {"E3 900/s: r2 300 ms slow; up to 3 attempts, retries to another replica", 900, false, 0.300, 3, true, 1e9},
        {"E4 1500/s: as E2 at peak traffic", 1500, false, 0.300, 3, false, 1e9},
        {"E5 1500/s: as E4 with a retry budget of 10 % of new requests", 1500, false, 0.300, 3, false, 0.10},
    };
    for (const auto& e : ex) {
        run(e);
    }
    return 0;
}
