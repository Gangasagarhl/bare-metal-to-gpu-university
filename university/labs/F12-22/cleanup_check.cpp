// cleanup_check.cpp - forensic evidence: the team's unit tests, then the characterization
// comparison, both run on the cleaned-up formatter.
#include <iostream>

#include "inputs.hpp"
#include "legacy_seam.hpp"
#include "status_cleanup.hpp"

std::time_t fixedClock()
{
    return 1234567;
}

int main()
{
    legacy::g_clock = &fixedClock;
    cleanup::g_clock = &fixedClock;

    std::cout << "--- the unit tests that came with the code\n";
    const char* tests[][2] = {
        {"V=12.0;I=1.5;T=30", "12.0V 1.5A 30C @7"},
        {"V=10.0;I=-2;T=70", "10.0V CHG 2.0A 70C HOT LOW @7"},
        {"V=12.5", "12.5V 0.0A @7"},
    };
    for (const auto& t : tests) {
        const bool ok = cleanup::formatStatus(t[0]) == t[1];
        std::cout << (ok ? "ok      " : "FAILED  ") << t[0] << "\n";
    }

    std::cout << "--- characterization: legacy and cleanup, same calls in the same order\n";
    int same = 0;
    for (const std::string& in : kInputs) {
        const std::string want = legacy::formatStatus(in);
        const std::string got = cleanup::formatStatus(in);
        if (want == got) {
            ++same;
            continue;
        }
        std::cout << "input \"" << in << "\"\n    legacy:  " << want << "\n    cleanup: " << got
                  << "\n";
    }
    std::cout << same << " of " << kInputs.size() << " lines identical\n";
    return 0;
}
