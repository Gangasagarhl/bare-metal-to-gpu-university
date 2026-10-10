// F9-34 Listing 1 (course_sim.cpp): a wheeled robot drives a circle of radius 3 m in a room
// with three known landmarks. It writes ekf_log.csv at 10 samples per second for 60 s:
// odometry speed v and turn rate w, one range-bearing reading of one landmark (taking turns),
// and the ground-truth pose. All noise levels are simulator settings.
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

double wrap(double a)  // to (-pi, pi]
{
    const double pi = std::acos(-1.0);
    while (a > pi) {
        a -= 2.0 * pi;
    }
    while (a <= -pi) {
        a += 2.0 * pi;
    }
    return a;
}

int main()
{
    const double pi = std::acos(-1.0);
    const double dt = 0.1;
    const double lmx[3] = {4.0, -4.0, 0.0};
    const double lmy[3] = {4.0, 2.0, -5.0};
    const double vSd = 0.05, wSd = 0.05;                    // odometry noise
    const double rangeSd = 0.1, bearingSd = 2.0 * pi / 180.0;  // sensor noise
    std::mt19937 engine(34);

    double x = 3.0, y = 0.0, th = pi / 2.0;  // start on the circle, facing along it
    std::ofstream out("ekf_log.csv");
    out << "t_s,v_odo,w_odo,landmark,range_m,bearing_rad,truth_x,truth_y,truth_th\n" << std::fixed
        << std::setprecision(4);
    for (int k = 1; k <= 600; ++k) {
        const double v = 0.5, w = 0.5 / 3.0;  // true commands: a circle of radius 3 m
        x += v * dt * std::cos(th);
        y += v * dt * std::sin(th);
        th = wrap(th + w * dt);
        const int id = k % 3;
        const double dx = lmx[id] - x, dy = lmy[id] - y;
        const double r = std::sqrt(dx * dx + dy * dy) + rangeSd * gaussian(engine);
        const double b = wrap(std::atan2(dy, dx) - th + bearingSd * gaussian(engine));
        out << std::setprecision(1) << k * dt << std::setprecision(4) << ','
            << v + vSd * gaussian(engine) << ',' << w + wSd * gaussian(engine) << ',' << id << ','
            << r << ',' << b << ',' << x << ',' << y << ',' << th << '\n';
    }
    std::cout << "wrote ekf_log.csv (600 samples)\n";
    std::ifstream check("ekf_log.csv");
    std::string line;
    for (int i = 0; i < 4 && std::getline(check, line); ++i) {
        std::cout << line << '\n';
    }
    return 0;
}
