// F9-20 forensic evidence generator: "The tuning report that passed".
// A colleague's report tool (its settling-time function is below) and the raw
// position log that the report was made from. The fault is explained in the answer key.
#include "cart_sim.hpp"
#include <cmath>
#include <format>
#include <iostream>
#include <vector>

// Report tool: settling time from a trace sampled every 0.1 s.
double reportSettlingTime(const std::vector<double>& trace, double setpoint)
{
    for (std::size_t i = 0; i < trace.size(); ++i) {
        if (std::abs(trace[i] - setpoint) <= 0.02 * setpoint) {
            return i * 0.1;
        }
    }
    return -1.0;
}

int main()
{
    const Gains g{6.0, 1.5, 4.5};
    std::vector<double> trace;
    const Metrics m = simulate(g, 10.0, &trace);
    const double settle = reportSettlingTime(trace, 1.0);
    std::cout << "TUNING REPORT  gains Kp 6.0, Ki 1.5, Kd 4.5\n";
    std::cout << std::format("  S1 overshoot      {:>6.1f} %   limit 5 %     {}\n", m.overshoot,
                             m.overshoot <= 5.0 ? "PASS" : "FAIL");
    std::cout << std::format("  S2 settling time  {:>6.2f} s   limit 3.0 s   {}\n", settle,
                             settle <= 3.0 ? "PASS" : "FAIL");
    std::cout << std::format("  S3 final error    {:>6.1f} mm  limit 2 mm    {}\n",
                             m.finalError * 1000.0, m.finalError <= 0.002 ? "PASS" : "FAIL");
    std::cout << std::format("  S4 peak asked     {:>6.2f} N   limit 10 N    {}\n", m.peakAsked,
                             m.peakAsked <= 10.0 ? "PASS" : "FAIL");
    std::cout << "\nRAW LOG (position every 0.1 s, from t = 1.5 s)\n  t (s)   x (m)\n";
    for (std::size_t i = 15; i <= 45 && i < trace.size(); ++i) {
        std::cout << std::format("{:>7.1f} {:>7.4f}\n", i * 0.1, trace[i]);
    }
    return 0;
}
