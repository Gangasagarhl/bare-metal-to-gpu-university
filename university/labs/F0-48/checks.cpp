// F0-48 number check: recomputes every number used in the chapter text.
#include <cmath>
#include <iomanip>
#include <iostream>
#include <numbers>

struct Vec3
{
    double x;
    double y;
    double z;
};

double dot(Vec3 a, Vec3 b)
{
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

double length(Vec3 a)
{
    return std::sqrt(dot(a, a));
}

double deg(double radians)
{
    return radians * 180.0 / std::numbers::pi;
}

int main()
{
    std::cout << std::fixed << std::setprecision(4);
    std::cout << "|(3,4)| = " << length({3, 4, 0}) << ", |(2,3,6)| = " << length({2, 3, 6})
              << ", |(1,1)| = " << length({1, 1, 0}) << "\n";
    std::cout << "(1,2,3).(4,-5,6) = " << dot({1, 2, 3}, {4, -5, 6}) << "\n";
    std::cout << "(1,0).(3,4) = " << dot({1, 0, 0}, {3, 4, 0}) << ", acos(3/5) = "
              << deg(std::acos(0.6)) << " deg\n";
    std::cout << "(1,0).(-2,2) = " << dot({1, 0, 0}, {-2, 2, 0}) << ", angle = "
              << deg(std::acos(-2.0 / length({-2, 2, 0}))) << " deg\n";
    std::cout << "(2,1).(-1,2) = " << dot({2, 1, 0}, {-1, 2, 0}) << "\n";
    std::cout << "cos 0, 60, 90, 120, 180 deg: " << std::cos(0.0) << " "
              << std::cos(std::numbers::pi / 3) << " " << std::cos(std::numbers::pi / 2) << " "
              << std::cos(2 * std::numbers::pi / 3) << " " << std::cos(std::numbers::pi) << "\n";
    std::cout << "pi = " << std::numbers::pi << ", 1 rad = " << deg(1.0) << " deg\n";
    // Worked example: wind and a drone's flight direction
    const Vec3 d{0.6, 0.8, 0.0};
    const Vec3 w{2.0, -1.0, 0.0};
    const double along = dot(w, d);
    const Vec3 cross{w.x - along * d.x, w.y - along * d.y, w.z - along * d.z};
    std::cout << "|d| = " << length(d) << ", w.d = " << along << ", crosswind = (" << cross.x << ", "
              << cross.y << ", " << cross.z << "), |crosswind| = " << length(cross)
              << ", |w|^2 = " << dot(w, w) << ", along^2 + cross^2 = "
              << along * along + dot(cross, cross) << ", crosswind.d = " << dot(cross, d) << "\n";
    std::cout << "angle between w and d = " << deg(std::acos(along / length(w))) << " deg\n";
    // Check yourself
    std::cout << "|(6,8)| = " << length({6, 8, 0}) << ", |(1,2,2)| = " << length({1, 2, 2})
              << ", (2,-3).(4,1) = " << dot({2, -3, 0}, {4, 1, 0}) << ", (3,4)/5 = (" << 3 / 5.0
              << ", " << 4 / 5.0 << ")\n";
    std::cout << "distance (1,1)->(4,5) = " << length({3, 4, 0}) << ", (0,1).(-0.5,-2) = "
              << dot({0, 1, 0}, {-0.5, -2, 0}) << "\n";
    std::cout << "cos(45 deg) = " << std::cos(std::numbers::pi / 4) << ", cos(30 deg) = "
              << std::cos(std::numbers::pi / 6) << "\n";
    std::cout << "|(4,2)| = " << length({4, 2, 0}) << ", (1,0).(4,2)/|(4,2)| = "
              << 4.0 / length({4, 2, 0}) << ", 45 deg in rad = " << std::numbers::pi / 4
              << ", cos(sofa) = " << -2.0 / length({-2, 2, 0}) << ", |(-2,2)| = "
              << length({-2, 2, 0}) << ", 0.4/sqrt(5) = " << 0.4 / std::sqrt(5.0) << "\n";
    return 0;
}
