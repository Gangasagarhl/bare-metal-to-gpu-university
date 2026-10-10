// F9-30 forensic evidence generator: "The robot that was always sure".
// A dead-reckoning module reports its position with a fixed sd of 0.05 m (the per-step
// noise), never growing it. 500 simulated runs of 25 steps compare the reported sd with
// the real error. Simulator settings: step 1.00 m, step-length noise sd 0.05 m.
#include <cmath>
#include <iomanip>
#include <iostream>
#include <random>
#include <vector>

double uniform01(std::mt19937& engine)
{
    return (static_cast<double>(engine()) + 0.5) / 4294967296.0;
}

double gaussian(std::mt19937& engine)
{
    const double u1 = uniform01(engine);
    const double u2 = uniform01(engine);
    return std::sqrt(-2.0 * std::log(u1)) * std::cos(2.0 * std::acos(-1.0) * u2);
}

int main()
{
    const int runs = 500;
    const int steps = 25;
    const double stepSd = 0.05;
    const double reportedSd = 0.05;  // what the module prints, at every step
    std::mt19937 engine(130);

    std::vector<std::vector<double>> error(steps + 1, std::vector<double>(runs, 0.0));
    for (int r = 0; r < runs; ++r) {
        double truth = 0.0;
        double estimate = 0.0;
        for (int k = 1; k <= steps; ++k) {
            truth += 1.0 + stepSd * gaussian(engine);  // what the wheels really did
            estimate += 1.0;                            // what the module assumes
            error[k][r] = estimate - truth;
        }
    }

    std::cout << "step  reported sd  RMS error  runs with |error| > 2 x reported sd\n";
    for (int k = 1; k <= steps; k += (k < 5 ? 1 : 5)) {
        double ss = 0.0;
        int outside = 0;
        for (double e : error[k]) {
            ss += e * e;
            if (std::fabs(e) > 2.0 * reportedSd) {
                ++outside;
            }
        }
        std::cout << std::setw(4) << k << std::fixed << std::setprecision(3) << std::setw(12)
                  << reportedSd << std::setw(11) << std::sqrt(ss / runs) << std::setw(10)
                  << outside << " of " << runs << " (" << std::setprecision(1)
                  << 100.0 * outside / runs << " %)\n";
    }
    return 0;
}
