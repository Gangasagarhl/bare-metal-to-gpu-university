// refactor_check.cpp - the refactored formatter against the legacy one, call for call, then the
// tests of the sprouted feature. Both formatters get the same fixed clock.
#include <cmath>
#include <iostream>

#include "inputs.hpp"
#include "legacy_seam.hpp"
#include "status_v2.hpp"

std::time_t fixedClock()
{
    return 1234567;
}

int main()
{
    legacy::g_clock = &fixedClock;
    v2::Formatter f(&fixedClock);
    int same = 0;
    for (const std::string& in : kInputs) {
        const std::string want = legacy::formatStatus(in);
        const std::string got = f.format(in);
        if (want == got) {
            ++same;
        } else {
            std::cout << "DIFF for \"" << in << "\": legacy \"" << want << "\" v2 \"" << got
                      << "\"\n";
        }
    }
    std::cout << "golden master: " << same << " of " << kInputs.size() << " lines identical\n";

    int fails = 0;
    auto check = [&fails](bool ok, const char* what) {
        std::cout << (ok ? "ok      " : "FAILED  ") << what << "\n";
        if (!ok) ++fails;
    };
    check(std::fabs(v2::cellVolts(12.6, 3) - 4.2) < 1e-9, "cellVolts(12.6, 3) == 4.2");
    check(v2::cellVolts(12.6, 0) == 12.6, "cellVolts with 0 cells returns the pack voltage");
    v2::Formatter g(&fixedClock);
    check(g.format("V=12.6;T=30", 3) == "12.6V 0.0A 30C 4.20V/cell @7", "format with cells=3");
    check(g.format("V=12.6;T=30") == "12.6V 0.0A 30C @7", "format without cells: unchanged");
    std::cout << fails << " new-feature check(s) failed\n";
    return (same == int(kInputs.size()) && fails == 0) ? 0 : 1;
}
