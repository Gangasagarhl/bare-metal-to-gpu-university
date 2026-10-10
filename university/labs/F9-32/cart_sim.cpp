// F9-32 Listing 1: the university's cart-on-a-rail simulator. It writes two "recordings":
//   rest_log.csv : 300 range readings with the cart parked at a tape-measured 2.000 m
//   cart_log.csv : 20 s of driving at 10 samples per second: time, wheel-odometry speed,
//                  range reading, and the ground-truth position (from the simulator)
// All noise levels are SIMULATOR SETTINGS, not the values of any real sensor.
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

// The commanded speed profile of the drive (m/s).
double trueSpeed(double t)
{
    if (t < 5.0) {
        return 0.5;
    }
    if (t < 8.0) {
        return 0.0;
    }
    if (t < 14.0) {
        return -0.3;
    }
    return 0.2;
}

int main()
{
    const double rangeSd = 0.08;  // m, range sensor noise (simulator setting)
    const double odoSd = 0.2;     // m/s, wheel-odometry speed noise (simulator setting)
    const double dt = 0.1;        // s
    std::mt19937 engine(32);

    std::ofstream rest("rest_log.csv");
    rest << "range_m\n" << std::fixed << std::setprecision(3);
    for (int i = 0; i < 300; ++i) {
        rest << 2.0 + rangeSd * gaussian(engine) << '\n';
    }

    std::ofstream log("cart_log.csv");
    log << "t_s,odo_speed_mps,range_m,truth_m\n" << std::fixed << std::setprecision(3);
    double x = 0.5;
    for (int k = 1; k <= 200; ++k) {
        const double t = k * dt;
        const double v = trueSpeed(t - dt);         // speed held during the last interval
        x += v * dt;                                 // ground truth
        const double odo = v + odoSd * gaussian(engine);
        const double z = x + rangeSd * gaussian(engine);
        log << std::setprecision(1) << t << ',' << std::setprecision(3) << odo << ',' << z << ','
            << x << '\n';
    }
    std::cout << "wrote rest_log.csv (300 lines) and cart_log.csv (200 lines)\n";
    std::ifstream check("cart_log.csv");
    std::string line;
    for (int i = 0; i < 4 && std::getline(check, line); ++i) {
        std::cout << line << '\n';
    }
    return 0;
}
