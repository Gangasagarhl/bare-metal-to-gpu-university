// budget.cpp - generates the evidence pack of the F5-42 forensic lab "the budget is gone and
// nobody was paged". The month comes from slo_model.hpp (forensicIncidents, seed 7); the alert
// rule that was deployed is replayed minute by minute to produce the alert history. What
// changed in the service is in the deploy log; the analysis is the learner's.
#include "slo_model.hpp"

#include <cstdio>
#include <string>

std::string when(int m)
{
    char b[40];
    std::snprintf(b, sizeof b, "day %02d %02d:%02d", m / kMinutesPerDay + 1, (m % kMinutesPerDay) / 60, m % 60);
    return b;
}

int main()
{
    const auto month = generateMonth(forensicIncidents(), 7);
    const Prefix p(month);

    std::printf("=== slo.md (excerpt) ===\n");
    std::printf("SLI: fraction of requests answered without a 5xx status, measured at the gateway\n");
    std::printf("SLO: 99.9 %% over a 30-day window (this month: days 01-30)\n\n");

    std::printf("=== alert-rules.conf (as deployed) ===\n");
    std::printf("page  KVErrorsHigh  when error_ratio over 5m > 2 %%  for 5 consecutive minutes\n\n");

    std::printf("=== alert history (replay of the rule above over the month) ===\n");
    int consecutive = 0;
    bool firing = false;
    int firedAt = 0;
    for (int m = 0; m < kMonth; ++m) {
        const bool cond = p.ratio(m, 5) > 0.02;
        consecutive = cond ? consecutive + 1 : 0;
        if (!firing && consecutive >= 5) {
            firing = true;
            firedAt = m;
            std::printf("%s  FIRING   KVErrorsHigh (error_ratio 5m = %.2f %%)\n", when(m).c_str(), 100.0 * p.ratio(m, 5));
        }
        if (firing && !cond) {
            firing = false;
            std::printf("%s  RESOLVED KVErrorsHigh (after %d min)\n", when(m).c_str(), m - firedAt);
        }
    }

    std::printf("\n=== deploy.log ===\n");
    std::printf("%s  deploy kv-gateway v2.30 -> v2.31 (\"retry once on upstream 503\" removed; cleanup)\n",
                when(at(14, 9, 12)).c_str());
    std::printf("%s  rollback kv-gateway v2.31 -> v2.30 (support ticket 4471)\n", when(at(27, 15, 40)).c_str());

    std::printf("\n=== support ticket 4471 ===\n");
    std::printf("%s  \"About 1 request in 400 fails with 503 since roughly two weeks; retrying works.\"\n",
                when(at(27, 11, 5)).c_str());

    std::printf("\n=== daily SLI (from the SLO report; nobody was assigned to read it) ===\n");
    std::printf("day   requests   failed   SLI %%\n");
    for (int d = 0; d < kDays; ++d) {
        long r = 0;
        long e = 0;
        for (int m = d * kMinutesPerDay; m < (d + 1) * kMinutesPerDay; ++m) {
            r += month[static_cast<std::size_t>(m)].requests;
            e += month[static_cast<std::size_t>(m)].errors;
        }
        std::printf("%02d  %9ld  %7ld   %.4f\n", d + 1, r, e, 100.0 * (1.0 - static_cast<double>(e) / static_cast<double>(r)));
    }
    return 0;
}
