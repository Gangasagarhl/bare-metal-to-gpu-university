// F0-52 number check: recomputes every number used in the chapter text.
#include <cmath>
#include <iomanip>
#include <iostream>
#include <numbers>

double rad(double d)
{
    return d * std::numbers::pi / 180.0;
}

double deg(double r)
{
    return r * 180.0 / std::numbers::pi;
}

int main()
{
    std::cout << std::fixed << std::setprecision(4);
    std::cout << "sin 0, 30, 45, 60, 90: " << std::sin(0.0) << " " << std::sin(rad(30)) << " " << std::sin(rad(45))
              << " " << std::sin(rad(60)) << " " << std::sin(rad(90)) << "\n";
    std::cout << "cos 30 = " << std::cos(rad(30)) << ", cos^2 + sin^2 at 30 = "
              << std::cos(rad(30)) * std::cos(rad(30)) + std::sin(rad(30)) * std::sin(rad(30)) << "\n";
    // worked example: robot at heading 0 turns 30 deg; lidar point (2,1)
    const double c = std::cos(rad(30)), s = std::sin(rad(30));
    std::cout << "R(30)(2,1) = (" << c * 2 - s * 1 << ", " << s * 2 + c * 1 << ")\n";
    std::cout << "R(30)(2,0) = (" << c * 2 << ", " << s * 2 << "), R(30)(0,1) = (" << -s << ", " << c << ")\n";
    std::cout << "|(2,1)| = " << std::hypot(2.0, 1.0) << ", heading of (2,1) = " << deg(std::atan2(1.0, 2.0))
              << " deg, +30 = " << deg(std::atan2(1.0, 2.0)) + 30 << "\n";
    std::cout << "undo: R(-30)(" << c * 2 - s << ", " << s * 2 + c << ") = (" << c * (c * 2 - s) + s * (s * 2 + c)
              << ", " << -s * (c * 2 - s) + c * (s * 2 + c) << ")\n";
    // radians-as-degrees bug
    for (double cmd : {30.0, 90.0, 180.0, 360.0}) {
        double wrapped = std::fmod(deg(cmd), 360.0);
        std::cout << cmd << " rad = " << deg(cmd) << " deg = " << wrapped << " deg after whole turns; ";
    }
    std::cout << "\n";
    // check yourself
    std::cout << "R(90)(3,1) = (" << -1.0 << ", " << 3.0 << "); R(180)(3,1) = (-3, -1)\n";
    std::cout << "R(45)(1,0) = (" << std::cos(rad(45)) << ", " << std::sin(rad(45)) << "); R(60)(2,0) = ("
              << 2 * std::cos(rad(60)) << ", " << 2 * std::sin(rad(60)) << ")\n";
    std::cout << "rotZ(90) e_x = e_y; rotX(90) e_y = e_z; rotY(90) e_z = e_x\n";
    std::cout << "120 deg in rad = " << rad(120) << ", 1 rad = " << deg(1.0) << " deg\n";
    return 0;
}
