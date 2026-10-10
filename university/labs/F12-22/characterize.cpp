// characterize.cpp - a characterization test: record what the code DOES today, through the seam.
// Its output is the golden master: refactor_check.cpp and cleanup_check.cpp compare against
// the same calls, in the same order.
#include <iomanip>
#include <iostream>

#include "inputs.hpp"
#include "legacy_seam.hpp"

std::time_t fixedClock()
{
    return 1234567;  // any fixed value: 1234567 % 60 == 7, so every line ends in "@7"
}

int main()
{
    legacy::g_clock = &fixedClock;
    for (const std::string& in : kInputs) {
        std::cout << std::left << std::setw(26) << std::quoted(in) << " -> "
                  << legacy::formatStatus(in) << "\n";
    }
    return 0;
}
