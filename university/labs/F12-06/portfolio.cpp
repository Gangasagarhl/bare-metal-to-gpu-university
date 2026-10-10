// portfolio.cpp - run every test level against the correct system and five mutants,
// and print which level catches which planted bug, and what each level cost.
#include "portfolio.hpp"

#include <cstdio>

int main()
{
    using namespace portfolio;
    std::printf("%-3s %-42s %-8s %-8s %-8s %-8s\n", "", "planted bug", "unit-P", "unit-G",
                "integr.", "system");
    long cost[4] = {0, 0, 0, 0};
    bool ok_clean = true;
    int caught_total = 0;
    for (const Variant& v : kVariants) {
        const Result r[4] = {unit_parser(v), unit_governor(v), integration(v), system_test(v)};
        bool caught = false;
        std::printf("%-3s %-42s", v.name, v.bug);
        for (int i = 0; i < 4; ++i) {
            std::printf(" %-8s", r[i].failed == 0 ? "pass" : "FAIL");
            caught = caught || r[i].failed > 0;
            if (&v == &kVariants[0]) {
                cost[i] = r[i].work;
            }
        }
        std::printf("\n");
        if (&v == &kVariants[0]) {
            ok_clean = !caught;
        } else if (caught) {
            ++caught_total;
        }
    }
    std::printf("\nwork per run of each level on the correct system:\n");
    std::printf("  unit-P %ld calls, unit-G %ld calls, integration %ld calls, system %ld steps\n",
                cost[0], cost[1], cost[2], cost[3]);
    std::printf("correct system passes every level: %s\n", ok_clean ? "yes" : "NO");
    std::printf("mutants caught by at least one level: %d of 5\n", caught_total);
    return (ok_clean && caught_total == 5) ? 0 : 1;
}
