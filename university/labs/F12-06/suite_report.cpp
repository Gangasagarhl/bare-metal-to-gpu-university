// suite_report.cpp - forensic evidence: the test suite a team inherited, run against the
// five mutants of portfolio.hpp. For each test: its level, its cost and the mutants it kills.
#include "portfolio.hpp"

#include <cstdio>
#include <string>
#include <vector>

namespace {

struct Test {
    std::string name;
    const char* level;
    portfolio::Result (*run)(const portfolio::Variant&, double);
    double arg;
};

portfolio::Result gov_far(const portfolio::Variant& v, double)
{
    portfolio::Result r;
    r.failed = portfolio::near(v.gov(3.0), 1.0) ? 0 : 1;
    r.work = 1;
    return r;
}

portfolio::Result scenario(const portfolio::Variant& v, double start)
{
    return portfolio::system_test(v, start);
}

}  // namespace

int main()
{
    std::vector<Test> suite;
    suite.push_back({"gov_far_is_full_speed", "unit", gov_far, 0.0});
    for (int i = 0; i < 12; ++i) {
        const double start = 1.5 + 0.5 * i;
        char name[40];
        std::snprintf(name, sizeof name, "drive_from_%.1f_m", start);
        suite.push_back({name, "system", scenario, start});
    }
    std::printf("%-24s %-7s %7s  %s\n", "test", "level", "work", "mutants killed");
    long total = 0;
    bool killed_any[6] = {};
    for (const Test& t : suite) {
        std::string kills;
        long work = 0;
        for (int m = 0; m < 6; ++m) {
            const portfolio::Result r = t.run(portfolio::kVariants[m], t.arg);
            if (m == 0) {
                work = r.work;
                if (r.failed != 0) {
                    kills += "(fails on the correct system!) ";
                }
            } else if (r.failed != 0) {
                kills += std::string(portfolio::kVariants[m].name) + " ";
                killed_any[m] = true;
            }
        }
        total += work;
        std::printf("%-24s %-7s %7ld  %s\n", t.name.c_str(), t.level, work,
                    kills.empty() ? "-" : kills.c_str());
    }
    std::printf("\ntotal work for one run of the suite: %ld\n", total);
    std::printf("mutants never killed:");
    for (int m = 1; m < 6; ++m) {
        if (!killed_any[m]) {
            std::printf(" %s (%s)", portfolio::kVariants[m].name, portfolio::kVariants[m].bug);
        }
    }
    std::printf("\n");
    return 0;
}
