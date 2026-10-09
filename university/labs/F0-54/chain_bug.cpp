// F0-54 forensic evidence: "The cup that follows the robot".
// A robot drives past a cup standing still on a table. Its range sensor is mounted on the robot
// (frame S, 0.4 m ahead of the robot origin, turned 30 deg left). Every half metre the software
// estimates the cup's position on the room map. This program contains ONE deliberate mistake.
#include <array>
#include <cmath>
#include <cstddef>
#include <iomanip>
#include <iostream>
#include <numbers>

using Mat3 = std::array<std::array<double, 3>, 3>;
using Vec3 = std::array<double, 3>;

Mat3 operator*(const Mat3& a, const Mat3& b)
{
    Mat3 c{};
    for (std::size_t i = 0; i < 3; ++i)
        for (std::size_t j = 0; j < 3; ++j)
            for (std::size_t k = 0; k < 3; ++k)
                c[i][j] += a[i][k] * b[k][j];
    return c;
}

Vec3 operator*(const Mat3& a, const Vec3& v)
{
    Vec3 r{};
    for (std::size_t i = 0; i < 3; ++i)
        for (std::size_t k = 0; k < 3; ++k)
            r[i] += a[i][k] * v[k];
    return r;
}

double radians(double degrees) { return degrees * std::numbers::pi / 180.0; }
double degrees(double rad) { return rad * 180.0 / std::numbers::pi; }

Mat3 pose2(double x, double y, double thetaDeg)
{
    const double c = std::cos(radians(thetaDeg)), s = std::sin(radians(thetaDeg));
    return {{{c, -s, x}, {s, c, y}, {0, 0, 1}}};
}

Mat3 inverse2(const Mat3& T)
{
    const double c = T[0][0], s = T[1][0], x = T[0][2], y = T[1][2];
    return {{{c, s, -(c * x + s * y)}, {-s, c, -(-s * x + c * y)}, {0, 0, 1}}};
}

// --- simulator side (correct): what the sensor really measures --------------------------
Vec3 simulateSensor(const Mat3& T_WR, const Mat3& T_RS, const Vec3& cupW)
{
    return inverse2(T_WR * T_RS) * cupW;
}

// --- robot software side: turn the sensor's point into a map position -------------------
Vec3 cupOnMap(const Mat3& T_WR, const Mat3& T_RS, const Vec3& cupS)
{
    const Mat3 T_WS = T_RS * T_WR;
    return T_WS * cupS;
}

int main()
{
    const Vec3 cupW = {4.0, 2.5, 1.0};            // known only to the simulator
    const Mat3 T_RS = pose2(0.4, 0.0, 30.0);      // sensor mounting, from the robot's drawings
    std::cout << std::fixed << std::setprecision(2);
    std::cout << "sensor mount T_RS: x 0.40 m, y 0.00 m, angle 30.00 deg\n";
    std::cout << "     robot pose (map)       |  sensor reading    |  cup estimate (map)\n";
    std::cout << "     x      y   heading     |  range  bearing    |     x      y\n";
    const double poses[][3] = {{0.0, 0.0, 0.0}, {0.5, 0.0, 0.0}, {1.0, 0.0, 0.0}, {1.5, 0.0, 0.0},
                               {1.5, 0.5, 30.0}, {1.5, 1.0, 60.0}, {1.5, 1.5, 90.0}};
    for (const auto& p : poses) {
        const Mat3 T_WR = pose2(p[0], p[1], p[2]);
        const Vec3 cupS = simulateSensor(T_WR, T_RS, cupW);
        const double range = std::hypot(cupS[0], cupS[1]);
        const double bearing = degrees(std::atan2(cupS[1], cupS[0]));
        const Vec3 est = cupOnMap(T_WR, T_RS, cupS);
        std::cout << std::setw(6) << p[0] << std::setw(7) << p[1] << std::setw(9) << p[2] << "     |"
                  << std::setw(7) << range << std::setw(9) << bearing << "    |"
                  << std::setw(8) << est[0] << std::setw(7) << est[1] << "\n";
    }
    return 0;
}
