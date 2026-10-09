// gameday.cpp - generates the evidence pack of the F5-43 forensic lab "the game day that took
// the service down". The service, the clients and the fault use the fluid model of Listing 1
// (chaos.cpp), copied here in compact form: three replicas kv-0..kv-2 of 700 requests/s each,
// clients with a 250 ms timeout and up to 3 attempts. The scripted parts (the plan, the chat,
// the health checks) are written to match what the model computes. Times are 10:38-10:47.
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <string>

constexpr double kDt = 0.1;
constexpr double kCapacity = 700.0;
constexpr double kTimeout = 0.250;
constexpr double kBase = 0.002;
constexpr double kOffered = 1500.0;
constexpr double kFaultOn = 120.0;    // s after 10:38:00 -> 10:40:00
constexpr double kFaultOff = 380.0;   // 10:44:20, when the operator stopped the experiment
constexpr double kNoRetry = 450.0;    // 10:45:30, client config pushed: max_attempts = 1

std::string hms(double s)
{
    const int t = static_cast<int>(s);
    char b[32];
    std::snprintf(b, sizeof b, "10:%02d:%02d", 38 + t / 60, t % 60);
    return b;
}

double lateShare(double load, double delay)
{
    if (load >= kCapacity) {
        return 1.0;
    }
    const double spare = kTimeout - delay;
    return spare <= 0 ? 1.0 : std::exp(-(kCapacity - load) * spare);
}

int main()
{
    std::printf("=== gameday-plan.md (excerpt) ===\n");
    std::printf("experiment: disk stall on kv-2 (every fsync of kv-2's write-ahead log delayed by 2 s)\n");
    std::printf("hypothesis: the service survives the loss of one replica; success stays >= 99.5 %%\n");
    std::printf("evidence:   last month's experiment 'kill kv-2' (09:00, about 900 requests/s): success 100 %%\n");
    std::printf("when:       10:40, for 15 minutes      scope: production, all clients\n");
    std::printf("abort:      the operator stops the experiment if the dashboard looks bad\n\n");

    std::printf("=== client configuration (excerpt) ===\n");
    std::printf("timeout_ms = 250   max_attempts = 3   retry_target = any_replica   retry_budget = none\n\n");

    std::printf("=== metrics (30 s windows) ===\n");
    std::printf("window    new/s  attempts/s  success %%  kv-0 load  kv-1 load  kv-2 load  kv-2 fsync p99  kv-2 busy threads\n");
    std::array<std::array<double, 3>, 4> retry{};
    double wNew = 0;
    double wFail = 0;
    double wAtt = 0;
    std::array<double, 3> wLoad{};
    for (int s = 0; s < static_cast<int>(540.0 / kDt); ++s) {
        const double t = s * kDt;
        const bool fault = t >= kFaultOn && t < kFaultOff;
        std::array<std::array<double, 3>, 4> att{};
        for (int r = 0; r < 3; ++r) {
            att[1][static_cast<std::size_t>(r)] = kOffered * kDt / 3;
        }
        att[2] = retry[2];
        att[3] = retry[3];
        retry[2] = retry[3] = {};
        std::array<double, 3> load{};
        std::array<double, 3> late{};
        for (std::size_t r = 0; r < 3; ++r) {
            load[r] = (att[1][r] + att[2][r] + att[3][r]) / kDt;
            // the stall blocks every worker thread of kv-2, so reads wait behind writes too
            late[r] = (r == 2 && fault) ? 1.0 : lateShare(load[r], kBase);
        }
        for (std::size_t k = 1; k <= 3; ++k) {
            for (std::size_t r = 0; r < 3; ++r) {
                const double failed = att[k][r] * late[r];
                wAtt += att[k][r];
                const int attempts = t >= kNoRetry ? 1 : 3;
                if (static_cast<int>(k) < attempts) {
                    for (std::size_t q = 0; q < 3; ++q) {
                        retry[k + 1][q] += failed / 3;
                    }
                } else {
                    wFail += failed;
                }
            }
        }
        wNew += kOffered * kDt;
        for (std::size_t r = 0; r < 3; ++r) {
            wLoad[r] += load[r] / kCapacity;
        }
        if ((s + 1) % 300 == 0) {
            const double start = t + kDt - 30.0;
            const bool stalled = start + 30.0 > kFaultOn && start < kFaultOff;
            std::printf("%s  %6.0f  %10.0f  %9.2f  %8.0f%%  %8.0f%%  %8.0f%%  %11s  %17s\n", hms(start).c_str(),
                        wNew / 30.0, wAtt / 30.0, 100.0 * (1.0 - wFail / wNew), wLoad[0] / 3.0, wLoad[1] / 3.0, wLoad[2] / 3.0,
                        stalled ? "2004 ms" : "3 ms", (stalled || wLoad[2] / 3.0 > 100.0) ? "16 of 16" : "2 of 16");
            wNew = wFail = wAtt = 0;
            wLoad = {};
        }
    }

    std::printf("\n=== load-balancer health checks (TCP connect to port 7000 every 5 s) ===\n");
    std::printf("10:38:00-10:47:00  kv-0 OK  kv-1 OK  kv-2 OK   (no state changes)\n");

    std::printf("\n=== experiment.log ===\n");
    std::printf("%s  start: inject fsync delay 2000 ms on kv-2\n", hms(kFaultOn).c_str());
    std::printf("%s  stop: fault removed by operator\n", hms(kFaultOff).c_str());

    std::printf("\n=== pager.log ===\n");
    std::printf("%s  PAGE  KVAvailabilityFastBurn (burn(5m) > 14.4 and burn(1h) > 14.4)\n", hms(kFaultOn + 65).c_str());
    std::printf("%s  PAGE  KVGatewayErrors (all three upstreams timing out)\n", hms(kFaultOn + 71).c_str());
    std::printf("%s  RESOLVED  KVGatewayErrors\n", hms(kNoRetry + 31).c_str());

    std::printf("\n=== chat (game-day channel) ===\n");
    std::printf("%s  operator: fault is in, watching the dashboard\n", hms(kFaultOn + 5).c_str());
    std::printf("%s  operator: dashboard (1 min refresh) still shows the 10:39 minute, all green\n", hms(kFaultOn + 50).c_str());
    std::printf("%s  on-call: I just got paged for KV availability, is that you?\n", hms(kFaultOn + 80).c_str());
    std::printf("%s  operator: kv-2 is the only target and it passes health checks; kv-0 and kv-1 should be fine\n",
                hms(kFaultOn + 140).c_str());
    std::printf("%s  on-call: kv-0 and kv-1 are at 100%% CPU and timing out too\n", hms(kFaultOn + 205).c_str());
    std::printf("%s  operator: stopping the experiment\n", hms(kFaultOff - 2).c_str());
    std::printf("%s  on-call: fault is out, kv-2 fsync is back to 3 ms, but everything is still timing out\n",
                hms(kFaultOff + 40).c_str());
    std::printf("%s  on-call: pushing client config max_attempts = 1\n", hms(kNoRetry).c_str());
    std::printf("%s  on-call: success back to 100 %%; leaving max_attempts = 1 until the review\n",
                hms(kNoRetry + 35).c_str());
    return 0;
}
