// F0-62 Listing 1: the university's IMU-at-rest simulator. It writes imu_rest.csv:
// 1000 samples (10 s at 100 samples per second) of a 3-axis accelerometer (m/s^2) and a
// 3-axis gyroscope (deg/s) lying still and level. Every parameter below is a SIMULATOR
// SETTING chosen for teaching, not a property of any real sensor part.
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <random>
#include <sstream>

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

// The sensor reports whole multiples of its step (its resolution).
double quantise(double value, double step)
{
    return step * std::round(value / step);
}

int main()
{
    const double truth[6] = {0.0, 0.0, 9.81, 0.0, 0.0, 0.0};      // at rest, level
    const double bias[6] = {0.05, -0.03, 0.08, 0.30, -0.12, 0.05};
    const double sigma[6] = {0.02, 0.02, 0.03, 0.10, 0.10, 0.10};
    const double sharedVibration = 0.015;  // added equally to ax and ay (F0-60)
    const double accelStep = 0.005;
    const double gyroStep = 0.06;

    std::mt19937 engine(62);
    std::ofstream csv("imu_rest.csv");
    csv << "t_s,ax,ay,az,gx,gy,gz\n";
    std::cout << "first lines of imu_rest.csv:\nt_s,ax,ay,az,gx,gy,gz\n";
    for (int i = 0; i < 1000; ++i) {
        const double shared = sharedVibration * gaussian(engine);
        std::ostringstream row;
        row << std::fixed << std::setprecision(2) << i * 0.01;
        for (int axis = 0; axis < 6; ++axis) {
            double v = truth[axis] + bias[axis] + sigma[axis] * gaussian(engine);
            if (axis < 2) {
                v += shared;
            }
            const double step = axis < 3 ? accelStep : gyroStep;
            row << "," << std::setprecision(axis < 3 ? 3 : 2) << quantise(v, step);
        }
        csv << row.str() << "\n";
        if (i < 5) {
            std::cout << row.str() << "\n";
        }
    }
    std::cout << "...\nwrote imu_rest.csv: 1000 samples (10 s at 100 samples per second)\n";
    return 0;
}
