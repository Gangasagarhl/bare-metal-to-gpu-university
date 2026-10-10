// exposure.cpp: the cost of being wrong, as arithmetic (BR-09 worked example).
// A silent bug produces wrong results from the day it ships until every user runs
// the fix. Wrong results = users x runs per user per day x days exposed x fraction of
// runs whose input triggers the bug. The scenario numbers are a story, not data.
#include <cstdio>
#include <vector>

struct Scenario
{
    const char* name;
    long users;
    long runs_per_user_per_day;
    long days_to_notice;   // ship -> someone notices
    long days_to_fix;      // notice -> fixed release available
    long days_to_update;   // release -> the last user has updated
    long bad_per_10000;    // runs (out of 10,000) whose input triggers the bug
};

int main()
{
    const std::vector<Scenario> scenarios = {
        {"hobby: only you, you notice at once", 1, 5, 1, 1, 0, 30},
        {"production, nobody watching", 40, 50, 60, 2, 14, 30},
        {"production, error-rate alert", 40, 50, 1, 2, 14, 30},
        {"production, alert + fast update", 40, 50, 1, 2, 2, 30},
    };
    std::printf("%-38s %6s %8s %6s %13s\n", "scenario", "users", "runs/day", "days",
                "wrong results");
    for (const Scenario& s : scenarios) {
        const long runs_per_day = s.users * s.runs_per_user_per_day;
        const long days = s.days_to_notice + s.days_to_fix + s.days_to_update;
        const double wrong = static_cast<double>(runs_per_day * days * s.bad_per_10000) / 10000.0;
        std::printf("%-38s %6ld %8ld %6ld %13.2f\n", s.name, s.users, runs_per_day, days, wrong);
    }
    return 0;
}
