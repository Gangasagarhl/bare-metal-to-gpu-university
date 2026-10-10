// F9-35 Listing 1 (drive_sim.cpp): a wheeled robot drives for 120 s on a floor with two
// slippery patches. It writes drive_log.csv at 10 samples per second: wheel-odometry speed v
// and turn rate w_odo (from the two encoders), the IMU gyro's z rate, and the ground truth.
// The gyro has the noise level measured in MA202 (F0-62, about 0.1 deg/s) and a bias of
// 0.3 deg/s that this filter is NOT told. All values are simulator settings.
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

int main()
{
    const double pi = std::acos(-1.0);
    const double deg = pi / 180.0;
    const double dt = 0.1;
    const double gyroBias = 0.3 * deg, gyroSd = 0.1 * deg;  // rad/s
    const double odoWSd = 0.02, odoVSd = 0.01;              // rad/s, m/s
    std::mt19937 engine(35);
    std::ofstream out("drive_log.csv");
    out << "t_s,v_odo,w_odo,gyro_z,truth_x,truth_y,truth_th,slip\n" << std::fixed;
    double x = 0.0, y = 0.0, th = 0.0;
    for (int k = 1; k <= 1200; ++k) {
        const double t = k * dt;
        // Drive plan: straight, left arc, straight, right arc, stop, repeated with variations.
        const int phase = static_cast<int>(t) % 30;
        double v = 0.4, w = 0.0;
        if (phase >= 8 && phase < 14) {
            w = 0.3;
        } else if (phase >= 20 && phase < 26) {
            w = -0.25;
        } else if (phase >= 27) {
            v = 0.0;
        }
        x += v * dt * std::cos(th);
        y += v * dt * std::sin(th);
        th += w * dt;
        // Slip: on two patches one wheel spins, so the encoders report a turn that is not real.
        const bool slip = (t > 40.0 && t <= 41.5) || (t > 85.0 && t <= 86.0);
        const double wOdo = w + odoWSd * gaussian(engine) + (slip ? 0.6 : 0.0);
        const double vOdo = v + odoVSd * gaussian(engine) + (slip ? 0.15 : 0.0);
        const double gyro = w + gyroBias + gyroSd * gaussian(engine);
        out << std::setprecision(1) << t << std::setprecision(5) << ',' << vOdo << ',' << wOdo
            << ',' << gyro << ',' << x << ',' << y << ',' << th << ',' << (slip ? 1 : 0) << '\n';
    }
    std::cout << "wrote drive_log.csv (1200 samples, 120 s)\n";
    std::ifstream check("drive_log.csv");
    std::string line;
    for (int i = 0; i < 3 && std::getline(check, line); ++i) {
        std::cout << line << '\n';
    }
    return 0;
}
