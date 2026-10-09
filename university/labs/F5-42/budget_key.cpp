// budget_key.cpp - computes the facts behind the answer key of the F5-42 forensic lab (same
// month as budget.cpp): when the error budget ran out, the burn rate during the deploy, and when
// the multiwindow burn-rate rules of Listing 2 would have alerted.
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
    const auto inc = forensicIncidents();
    const auto month = generateMonth(inc, 7);
    const Prefix p(month);
    const double budget = 0.001 * static_cast<double>(p.req.back());
    std::printf("month: %ld requests, %ld failed; budget %.0f; used %.1f %%\n", p.req.back(), p.err.back(), budget,
                100.0 * static_cast<double>(p.err.back()) / budget);
    for (int m = 0; m < kMonth; ++m) {
        if (static_cast<double>(p.err[static_cast<std::size_t>(m) + 1]) > budget) {
            std::printf("budget exhausted at %s\n", when(m).c_str());
            break;
        }
    }
    const auto& d = inc[1];
    const double r = p.ratio(d.start + d.minutes - 1, d.minutes);
    std::printf("deploy period: error ratio %.3f %% = burn rate %.2f; budget used in it %.1f %%\n", 100 * r, r / 0.001,
                100.0 * static_cast<double>(p.err[static_cast<std::size_t>(d.start + d.minutes)] - p.err[static_cast<std::size_t>(d.start)]) / budget);
    struct R
    {
        const char* name;
        int longW;
        int shortW;
        double b;
    };
    for (const R& rule : {R{"C: burn(1h)>14.4 and burn(5m)>14.4", 60, 5, 14.4}, R{"D: burn(6h)>6 and burn(30m)>6", 360, 30, 6},
                          R{"E: burn(3d)>1 and burn(6h)>1 (ticket)", 4320, 360, 1}}) {
        int first = -1;
        for (int m = d.start; m < d.start + d.minutes && first < 0; ++m) {
            if (p.ratio(m, rule.longW) / 0.001 > rule.b && p.ratio(m, rule.shortW) / 0.001 > rule.b) {
                first = m;
            }
        }
        if (first < 0) {
            std::printf("%-40s never during the deploy period\n", rule.name);
        } else {
            std::printf("%-40s first at %s (%.1f h after the deploy)\n", rule.name, when(first).c_str(),
                        (first - d.start) / 60.0);
        }
    }
    return 0;
}
