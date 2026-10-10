// slo.cpp - an availability SLO, its error budget, and four alert rules replayed over one month
// of minute-by-minute request counts (F5-42, Listing 2). For each rule: how many alerts it
// raised, how many were not about any incident, and how long after each incident's start it
// fired. The incidents and rates come from slo_model.hpp (exercise values).
#include "slo_model.hpp"

#include <cstdio>
#include <functional>
#include <string>
#include <vector>

constexpr double kSlo = 0.999;           // 99.9 % of requests succeed, over the 30-day month
constexpr double kBudget = 1.0 - kSlo;   // allowed error ratio

struct Rule
{
    std::string name;
    std::function<bool(const Prefix&, int)> firing;   // evaluated once per minute
};

int main()
{
    const auto incidents = labIncidents();
    const auto month = generateMonth(incidents, 42);
    const Prefix p(month);
    const long total = p.req.back();
    const long errors = p.err.back();
    const double allowed = kBudget * static_cast<double>(total);

    std::printf("== SLO: %.1f %% of requests succeed over %d days\n", 100 * kSlo, kDays);
    std::printf("requests %ld, failed %ld, SLI %.4f %%\n", total, errors,
                100.0 * (1.0 - static_cast<double>(errors) / static_cast<double>(total)));
    std::printf("error budget %.0f failed requests; used %.1f %%\n", allowed, 100.0 * static_cast<double>(errors) / allowed);
    for (const auto& inc : incidents) {
        long e = 0;
        for (int m = inc.start; m < inc.start + inc.minutes; ++m) {
            e += month[static_cast<std::size_t>(m)].errors;
        }
        std::printf("  incident '%s' (day %d, %d min): %ld failed = %.1f %% of the budget\n", inc.name.c_str(),
                    inc.start / kMinutesPerDay + 1, inc.minutes, e, 100.0 * static_cast<double>(e) / allowed);
    }

    auto burn = [](const Prefix& q, int m, int len) { return q.ratio(m, len) / kBudget; };
    const std::vector<Rule> rules = {
        {"A: ratio(5m) > 0.1 %", [](const Prefix& q, int m) { return q.ratio(m, 5) > kBudget; }},
        {"B: ratio(1h) > 1 %", [](const Prefix& q, int m) { return q.ratio(m, 60) > 0.01; }},
        {"C: burn(1h) > 14.4 and burn(5m) > 14.4",
         [&](const Prefix& q, int m) { return burn(q, m, 60) > 14.4 && burn(q, m, 5) > 14.4; }},
        {"D: burn(6h) > 6 and burn(30m) > 6",
         [&](const Prefix& q, int m) { return burn(q, m, 360) > 6 && burn(q, m, 30) > 6; }},
        {"E: burn(3d) > 1 and burn(6h) > 1 (ticket)",
         [&](const Prefix& q, int m) { return burn(q, m, 4320) > 1 && burn(q, m, 360) > 1; }},
    };

    std::printf("\n== alert rules replayed minute by minute\n");
    std::printf("%-42s %7s %7s   first alert after incident start (minutes)\n", "rule", "alerts", "noise");
    for (const auto& rule : rules) {
        int alerts = 0;
        int noise = 0;
        std::vector<int> firstFire(incidents.size(), -1);
        bool was = false;
        for (int m = 0; m < kMonth; ++m) {
            const bool now = rule.firing(p, m);
            bool related = false;
            for (std::size_t i = 0; i < incidents.size(); ++i) {
                const auto& inc = incidents[i];
                if (now && m >= inc.start && m < inc.start + inc.minutes + 60) {
                    related = true;
                    if (firstFire[i] < 0) {
                        firstFire[i] = m - inc.start;
                    }
                }
            }
            if (now && !was) {   // a new alert starts (rising edge)
                ++alerts;
                noise += !related;
            }
            was = now;
        }
        std::printf("%-42s %7d %7d  ", rule.name.c_str(), alerts, noise);
        for (std::size_t i = 0; i < incidents.size(); ++i) {
            if (firstFire[i] < 0) {
                std::printf("  #%zu: never", i + 1);
            } else {
                std::printf("  #%zu: %d", i + 1, firstFire[i]);
            }
        }
        std::printf("\n");
    }
    std::printf("(noise = alerts that started outside every incident and the hour after it)\n");
    return 0;
}
