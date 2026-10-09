// F0-55 checks: recomputes every number used in the chapter text (worked example, check yourself, answer key).
#include <cmath>
#include <iomanip>
#include <iostream>
#include <numbers>

struct Quat { double w, x, y, z; };

double tidy(double v) { return std::abs(v) < 5e-13 ? 0.0 : v; }

Quat operator*(const Quat& a, const Quat& b)
{
    return {a.w * b.w - a.x * b.x - a.y * b.y - a.z * b.z,
            a.w * b.x + a.x * b.w + a.y * b.z - a.z * b.y,
            a.w * b.y - a.x * b.z + a.y * b.w + a.z * b.x,
            a.w * b.z + a.x * b.y - a.y * b.x + a.z * b.w};
}

Quat conj(const Quat& q) { return {q.w, -q.x, -q.y, -q.z}; }
double norm(const Quat& q) { return std::sqrt(q.w * q.w + q.x * q.x + q.y * q.y + q.z * q.z); }

Quat fromAxisAngle(double ax, double ay, double az, double deg)
{
    const double h = deg * std::numbers::pi / 360.0;
    return {std::cos(h), ax * std::sin(h), ay * std::sin(h), az * std::sin(h)};
}

void print(const char* name, const Quat& q)
{
    std::cout << name << " = (" << tidy(q.w) << ", " << tidy(q.x) << ", " << tidy(q.y) << ", " << tidy(q.z) << ")\n";
}

double headingDeg(const Quat& q)
{
    return std::atan2(2 * (q.x * q.y + q.w * q.z), 1 - 2 * (q.y * q.y + q.z * q.z)) * 180.0 / std::numbers::pi;
}

int main()
{
    std::cout << std::setprecision(4);
    // worked example: 90 deg about z applied to e_x, step by step
    const Quat q = fromAxisAngle(0, 0, 1, 90), v{0, 1, 0, 0};
    print("worked: q", q);
    print("worked: q * v", q * v);
    print("worked: conj(q)", conj(q));
    print("worked: (q * v) * conj(q)", (q * v) * conj(q));
    print("worked: q * q (two quarter turns)", q * q);
    print("worked: conj(q) v q (the other way round)", conj(q) * v * q);
    // check yourself
    print("Q1: 180 deg about x", fromAxisAngle(1, 0, 0, 180));
    print("Q1: rotates e_y to", fromAxisAngle(1, 0, 0, 180) * Quat{0, 0, 1, 0} * conj(fromAxisAngle(1, 0, 0, 180)));
    const Quat h{0.5, 0.5, 0.5, 0.5};
    std::cout << "Q3: |(0.5,0.5,0.5,0.5)| = " << norm(h) << ", angle = " << 2 * std::acos(0.5) * 180 / std::numbers::pi
              << " deg, axis = (1,1,1)/sqrt(3) = " << 1 / std::sqrt(3.0) << " each\n";
    print("Q3: rotates e_x to", h * v * conj(h));
    std::cout << "Q4: |(1,1,0,0)| = " << norm({1, 1, 0, 0}) << "\n";
    print("Q4: normalised", Quat{1 / std::sqrt(2.0), 1 / std::sqrt(2.0), 0, 0});
    std::cout << "counts: Hamilton product 16 multiplies + 12 adds; 3x3 matrix product 27 multiplies + 18 adds\n";
    // forensic key: first-order update multiplies |q| by sqrt(1 + (w dt / 2)^2) each step
    const double w = 50.0 * std::numbers::pi / 180.0, dt = 0.01;
    const double f = std::sqrt(1 + (w * dt / 2) * (w * dt / 2));
    std::cout << std::setprecision(8) << "forensic: growth per step = " << f << std::setprecision(4)
              << ", |q| after 6000 steps = " << std::pow(f, 6000) << ", |nose| = |q|^2 = " << std::pow(f, 12000) << "\n";
    const Quat logged{-0.8710, 0, 0, 0.5029};
    std::cout << "forensic: t = 6 row: |q| = " << norm(logged) << ", true heading of that q = "
              << 2 * std::atan2(logged.z, logged.w) * 180 / std::numbers::pi - 360 << ", shown heading = "
              << headingDeg(logged) << "\n";
    const double n = norm(logged);
    const Quat fixed{logged.w / n, 0, 0, logged.z / n};
    std::cout << "forensic: shown heading after normalising = " << headingDeg(fixed) << "\n";
    return 0;
}
