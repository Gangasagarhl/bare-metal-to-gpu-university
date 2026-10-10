// F9-33 Listing 1 (drive_sim.cpp): a robot moving along a line with random accelerations,
// seen by a position sensor. Writes track_log.csv (10 samples per second, 30 s) and
// track_fast.csv (the same kind of drive recorded at 20 samples per second, used by the
// forensic lab). Columns: time, position reading, true position, true velocity.
// All noise levels are simulator settings.
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <random>
#include <string>

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

void record(const std::string& path, double dt, int samples, unsigned seed)
{
    const double accelSd = 0.5;  // m/s^2, held constant during each interval
    const double posSd = 0.1;    // m, position sensor noise
    std::mt19937 engine(seed);
    std::ofstream out(path);
    out << "t_s,z_m,truth_p_m,truth_v_mps\n" << std::fixed;
    double p = 0.0, v = 0.0;
    for (int k = 1; k <= samples; ++k) {
        const double a = accelSd * gaussian(engine);
        p += v * dt + 0.5 * a * dt * dt;
        v += a * dt;
        const double z = p + posSd * gaussian(engine);
        out << std::setprecision(2) << k * dt << ',' << std::setprecision(4) << z << ',' << p
            << ',' << v << '\n';
    }
}

int main()
{
    record("track_log.csv", 0.1, 300, 33);
    record("track_fast.csv", 0.05, 600, 133);
    std::cout << "wrote track_log.csv (300 samples, dt 0.1 s) and track_fast.csv (600 samples, "
                 "dt 0.05 s)\n";
    std::ifstream check("track_log.csv");
    std::string line;
    for (int i = 0; i < 4 && std::getline(check, line); ++i) {
        std::cout << line << '\n';
    }
    return 0;
}
