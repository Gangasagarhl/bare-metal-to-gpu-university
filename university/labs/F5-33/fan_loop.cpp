// fan_loop.cpp - the F5-33 lab model run: where does the processor temperature settle for each
// load, with four working fans and with one failed fan, at two inlet temperatures?
// Uses thermal_model.h; all numbers are model numbers (see its header comment).
#include "thermal_model.h"

#include <iomanip>
#include <iostream>

int main()
{
    std::cout << "inlet  load  fans  settles at  fan duty   (after 60 simulated minutes)\n";
    for (double inlet : {22.0, 26.0}) {
        for (double load : {30.0, 60.0, 100.0}) {
            for (int working : {4, 3}) {
                Server s;
                for (int f = working; f < s.fans; ++f) {
                    s.fan_ok[static_cast<std::size_t>(f)] = false;
                }
                for (int m = 0; m < 60; ++m) {
                    s.minute(inlet, load);
                }
                std::cout << std::fixed << std::setprecision(0) << std::setw(4) << inlet << " C"
                          << std::setw(5) << load << " %" << std::setw(5) << working
                          << std::setprecision(1) << std::setw(10) << s.cpu_c << " C"
                          << std::setprecision(0) << std::setw(8) << s.duty << " %"
                          << (s.cpu_c >= 95.0 ? "   >= 95 C (model's upper critical)" : "") << '\n';
            }
        }
    }
    return 0;
}
