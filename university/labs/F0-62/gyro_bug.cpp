// F0-62 forensic evidence: the robot that turns while standing still.
// The firmware estimates the gyro z bias as the mean of the first 3 s after power-on,
// subtracts it, and integrates the rate into a heading. Simulated log, 100 samples/s.
#include <cmath>
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

void stats(const char* label, const std::vector<double>& v)
{
    double m = 0.0;
    for (double x : v) {
        m += x / static_cast<double>(v.size());
    }
    double ss = 0.0;
    for (double x : v) {
        ss += (x - m) * (x - m);
    }
    std::cout << label << ": n " << v.size() << ", mean " << m << " deg/s, sd "
              << std::sqrt(ss / static_cast<double>(v.size() - 1)) << " deg/s\n";
}

int main()
{
    const double pi = std::acos(-1.0);
    const double dt = 0.01;
    std::mt19937 engine(162);
    std::vector<double> calibration;
    std::vector<double> rest;
    for (int i = 0; i < 300; ++i) {          // 0-3 s: what really happened is in the key
        const double t = i * dt;
        const double trueRate = 6.0 * std::sin(pi * t);
        calibration.push_back(trueRate + 0.30 + 0.10 * gaussian(engine));
    }
    for (int i = 0; i < 6000; ++i) {         // 3-63 s: robot on the floor, not moving
        rest.push_back(0.30 + 0.10 * gaussian(engine));
    }
    stats("samples 0-299 (calibration window)", calibration);
    stats("samples 300-6299 (on the floor)   ", rest);

    double biasEstimate = 0.0;
    for (double r : calibration) {
        biasEstimate += r / static_cast<double>(calibration.size());
    }
    double heading = 0.0;
    std::cout << "estimated bias: " << biasEstimate << " deg/s\n";
    std::cout << "time (s)   heading (deg)\n";
    for (std::size_t i = 0; i < rest.size(); ++i) {
        heading += (rest[i] - biasEstimate) * dt;
        if ((i + 1) % 1000 == 0) {
            std::cout << 3 + (i + 1) / 100 << "         " << heading << "\n";
        }
    }
    return 0;
}
