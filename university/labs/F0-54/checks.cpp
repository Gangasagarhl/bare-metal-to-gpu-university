// F0-54 checks: recomputes every number used in the chapter text (worked example, check yourself, answer key).
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

double tidy(double v) { return std::abs(v) < 5e-13 ? 0.0 : v; }

Mat3 pose2(double x, double y, double thetaDeg)
{
    const double t = thetaDeg * std::numbers::pi / 180.0;
    return {{{tidy(std::cos(t)), tidy(-std::sin(t)), x}, {tidy(std::sin(t)), tidy(std::cos(t)), y}, {0, 0, 1}}};
}

Mat3 inverse2(const Mat3& T)
{
    const double c = T[0][0], s = T[1][0], x = T[0][2], y = T[1][2];
    return {{{c, s, -(c * x + s * y)}, {-s, c, -(-s * x + c * y)}, {0, 0, 1}}};
}

void print(const char* name, const Mat3& m)
{
    std::cout << name << " = [";
    for (std::size_t i = 0; i < 3; ++i) {
        std::cout << "[";
        for (std::size_t j = 0; j < 3; ++j) std::cout << tidy(m[i][j]) << (j < 2 ? ", " : "]");
        std::cout << (i < 2 ? ", " : "]\n");
    }
}

void print(const char* name, const Vec3& v)
{
    std::cout << name << " = (" << tidy(v[0]) << ", " << tidy(v[1]) << ", " << tidy(v[2]) << ")\n";
}

int main()
{
    std::cout << std::setprecision(4);
    // worked example: robot at (3, 1) heading 90; sensor 0.5 m ahead, not turned; cup 2 m ahead of the sensor
    const Mat3 T_WR = pose2(3, 1, 90), T_RS = pose2(0.5, 0, 0);
    const Vec3 cupS = {2, 0, 1};
    print("worked: T_WR", T_WR);
    print("worked: T_RS", T_RS);
    print("worked: T_WS = T_WR*T_RS", T_WR * T_RS);
    print("worked: cup in world", (T_WR * T_RS) * cupS);
    print("worked: wrong order T_RS*T_WR", T_RS * T_WR);
    print("worked: wrong-order cup", (T_RS * T_WR) * cupS);
    print("worked: T_SW = inverse", inverse2(T_WR * T_RS));
    print("worked: T_SW * cupW", inverse2(T_WR * T_RS) * ((T_WR * T_RS) * cupS));
    print("worked: forward direction (1,0,0) in world", (T_WR * T_RS) * Vec3{1, 0, 0});
    // check yourself
    print("Q2: pose (1,2,0) applied to (3,4,1)", pose2(1, 2, 0) * Vec3{3, 4, 1});
    print("Q3: translate(1,0)*rot90 applied to (1,0,1)", (pose2(1, 0, 0) * pose2(0, 0, 90)) * Vec3{1, 0, 1});
    print("Q3: rot90*translate(1,0) applied to (1,0,1)", (pose2(0, 0, 90) * pose2(1, 0, 0)) * Vec3{1, 0, 1});
    print("Q4: inverse of pose (3,1,90)", inverse2(pose2(3, 1, 90)));
    print("Q4: check pose*inverse", pose2(3, 1, 90) * inverse2(pose2(3, 1, 90)));
    print("Q5: direction (0,1,0) under pose (5,5,90)", pose2(5, 5, 90) * Vec3{0, 1, 0});
    std::cout << "Q6: 3D, R = I, t = (1,2,3), point (1,1,1) -> (2, 3, 4)\n";
    std::cout << "multiply-adds: 4x4 times 4x4 = " << 4 * 4 * 4 << ", 3x3 times point = " << 3 * 3
              << ", 4x4 times point = " << 4 * 4 << "\n";
    // forensic key: with heading 0 the buggy estimate is off by (R_RS - I) t_WR
    const Mat3 mount = pose2(0.4, 0, 30);
    for (double x : {0.5, 1.0, 1.5}) {
        const double c = mount[0][0], s = mount[1][0];
        std::cout << "forensic: robot at (" << x << ", 0): error (R_RS - I) t = (" << (c - 1) * x << ", " << s * x
                  << "), estimate (" << 4 + (c - 1) * x << ", " << 2.5 + s * x << ")\n";
    }
    return 0;
}
