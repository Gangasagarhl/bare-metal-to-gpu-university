// F9-30 Listing 2: why uncertainty grows when you only count steps (dead reckoning).
// 4000 simulated robots each try to drive 1.00 m per step. Part A: only the step length is
// noisy (sd 0.05 m); the spread of the end positions grows like sqrt(k). Part B: the heading
// is noisy too (sd 3 degrees per step); the cloud of end points bends into a banana shape
// and its mean is no longer where the commands point. All numbers are simulator settings.
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
    const int robots = 4000;
    const double stepSd = 0.05;                              // m per step
    const double headingSd = 3.0 * std::acos(-1.0) / 180.0;  // rad per step
    std::mt19937 engine(30);

    std::cout << "Part A: noisy step length only (1-D)\n";
    std::cout << " k   mean x (m)   sd x (m)   predicted sd = 0.05*sqrt(k)\n";
    std::vector<double> x(robots, 0.0);
    for (int k = 1; k <= 16; ++k) {
        for (double& xi : x) {
            xi += 1.0 + stepSd * gaussian(engine);
        }
        if (k == 1 || k == 4 || k == 9 || k == 16) {
            double mean = 0.0;
            for (double xi : x) {
                mean += xi;
            }
            mean /= robots;
            double ss = 0.0;
            for (double xi : x) {
                ss += (xi - mean) * (xi - mean);
            }
            const double sd = std::sqrt(ss / (robots - 1));
            std::cout << std::setw(2) << k << std::fixed << std::setprecision(3) << std::setw(12)
                      << mean << std::setw(11) << sd << std::setw(12) << stepSd * std::sqrt(k)
                      << '\n';
        }
    }

    std::cout << "\nPart B: noisy step length and noisy heading (2-D), 20 steps\n";
    double sumX = 0.0;
    double sumY = 0.0;
    double sumXX = 0.0;
    double sumYY = 0.0;
    int behindHalf = 0;
    for (int r = 0; r < robots; ++r) {
        double px = 0.0;
        double py = 0.0;
        double heading = 0.0;
        for (int k = 0; k < 20; ++k) {
            heading += headingSd * gaussian(engine);
            const double len = 1.0 + stepSd * gaussian(engine);
            px += len * std::cos(heading);
            py += len * std::sin(heading);
        }
        sumX += px;
        sumY += py;
        sumXX += px * px;
        sumYY += py * py;
        if (px < 19.5) {
            ++behindHalf;
        }
    }
    const double mx = sumX / robots;
    const double my = sumY / robots;
    std::cout << std::fixed << std::setprecision(3) << "commanded end point: (20.000, 0.000)\n"
              << "mean end point:      (" << mx << ", " << my << ")\n"
              << "sd along x: " << std::sqrt(sumXX / robots - mx * mx)
              << " m   sd along y: " << std::sqrt(sumYY / robots - my * my) << " m\n"
              << "robots that ended more than 0.5 m short in x: " << behindHalf << " of " << robots
              << '\n';
    return 0;
}
