// F0-55 Listing 2: integrating gyro rates into an attitude quaternion (simulated drone, no hardware).
// The simulated gyro reports constant body rates. Each step multiplies the attitude by a small
// body-frame rotation (q <- q * dq) and renormalises. Expected final attitudes are known exactly.
#include <algorithm>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <numbers>

struct Vec3 { double x, y, z; };
struct Quat { double w, x, y, z; };

double radians(double d) { return d * std::numbers::pi / 180.0; }
double tidy(double v) { return std::abs(v) < 5e-13 ? 0.0 : v; }

Quat fromAxisAngle(Vec3 axis, double angleDeg)
{
    const double h = radians(angleDeg) / 2.0, s = std::sin(h);
    return {std::cos(h), axis.x * s, axis.y * s, axis.z * s};
}

Quat operator*(const Quat& a, const Quat& b)
{
    return {a.w * b.w - a.x * b.x - a.y * b.y - a.z * b.z,
            a.w * b.x + a.x * b.w + a.y * b.z - a.z * b.y,
            a.w * b.y - a.x * b.z + a.y * b.w + a.z * b.x,
            a.w * b.z + a.x * b.y - a.y * b.x + a.z * b.w};
}

Quat conj(const Quat& q) { return {q.w, -q.x, -q.y, -q.z}; }
double norm(const Quat& q) { return std::sqrt(q.w * q.w + q.x * q.x + q.y * q.y + q.z * q.z); }
Quat normalised(const Quat& q) { const double n = norm(q); return {q.w / n, q.x / n, q.y / n, q.z / n}; }

Vec3 rotate(const Quat& q, Vec3 v)
{
    const Quat r = q * Quat{0, v.x, v.y, v.z} * conj(q);
    return {r.x, r.y, r.z};
}

// one step with body rates w (degrees per second) for dt seconds
Quat step(const Quat& q, Vec3 wDeg, double dt)
{
    const double rate = std::sqrt(wDeg.x * wDeg.x + wDeg.y * wDeg.y + wDeg.z * wDeg.z);
    if (rate == 0.0) return q;
    const Vec3 axis{wDeg.x / rate, wDeg.y / rate, wDeg.z / rate};
    return normalised(q * fromAxisAngle(axis, rate * dt));  // body-frame increment: multiply on the right
}

// largest component difference between two attitudes, allowing for q and -q being the same rotation
double attitudeError(const Quat& a, const Quat& b)
{
    double plus = 0, minus = 0;
    const double da[] = {a.w - b.w, a.x - b.x, a.y - b.y, a.z - b.z};
    const double sa[] = {a.w + b.w, a.x + b.x, a.y + b.y, a.z + b.z};
    for (int i = 0; i < 4; ++i) {
        plus = std::max(plus, std::abs(da[i]));
        minus = std::max(minus, std::abs(sa[i]));
    }
    return std::min(plus, minus);
}

int main()
{
    std::cout << std::fixed << std::setprecision(4);
    const double dt = 0.01;  // 100 gyro samples per second (simulated)
    Quat q{1, 0, 0, 0};      // start level, nose along world x

    // phase 1: roll at 30 deg/s for 3 s; phase 2: yaw (body z) at 90 deg/s for 1 s
    const struct { Vec3 rate; int steps; const char* what; } phases[] = {
        {{30, 0, 0}, 300, "roll 30 deg/s for 3 s"}, {{0, 0, 90}, 100, "body yaw 90 deg/s for 1 s"}};
    for (const auto& ph : phases) {
        for (int k = 0; k < ph.steps; ++k) q = step(q, ph.rate, dt);
        const Vec3 nose = rotate(q, {1, 0, 0}), up = rotate(q, {0, 0, 1});
        std::cout << "after " << ph.what << ": q = (" << tidy(q.w) << ", " << tidy(q.x) << ", " << tidy(q.y) << ", "
                  << tidy(q.z) << "), |q| = " << norm(q) << "\n";
        std::cout << "    nose (body x) in world = (" << tidy(nose.x) << ", " << tidy(nose.y) << ", " << tidy(nose.z)
                  << "), body z in world = (" << tidy(up.x) << ", " << tidy(up.y) << ", " << tidy(up.z) << ")\n";
    }
    const Quat expected = fromAxisAngle({1, 0, 0}, 90) * fromAxisAngle({0, 0, 1}, 90);
    const double err = attitudeError(q, expected);
    std::cout << "expected roll90 * yaw90 = (" << expected.w << ", " << expected.x << ", " << expected.y << ", "
              << expected.z << ")\n";
    std::cout << "largest component difference from expected = " << std::scientific << err << "\n";
    return err < 1e-9 ? 0 : 1;
}
