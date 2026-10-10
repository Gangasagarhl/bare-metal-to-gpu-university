// F0-55 forensic evidence: "The compass that drifts".
// A simulated drone yaws at a constant 50 deg/s on a test stand for 60 s. The attitude is updated
// from the gyro 100 times per second with the first-order rule q <- q + (dt/2) q * (0, w).
// This program contains ONE deliberate mistake.
#include <cmath>
#include <iomanip>
#include <iostream>
#include <numbers>

struct Vec3 { double x, y, z; };
struct Quat { double w, x, y, z; };

Quat operator*(const Quat& a, const Quat& b)
{
    return {a.w * b.w - a.x * b.x - a.y * b.y - a.z * b.z,
            a.w * b.x + a.x * b.w + a.y * b.z - a.z * b.y,
            a.w * b.y - a.x * b.z + a.y * b.w + a.z * b.x,
            a.w * b.z + a.x * b.y - a.y * b.x + a.z * b.w};
}

Quat conj(const Quat& q) { return {q.w, -q.x, -q.y, -q.z}; }

Vec3 rotate(const Quat& q, Vec3 v)
{
    const Quat r = q * Quat{0, v.x, v.y, v.z} * conj(q);
    return {r.x, r.y, r.z};
}

// heading shown on the ground station: from the rotation-matrix formula for a unit quaternion
double headingDeg(const Quat& q)
{
    const double r00 = 1 - 2 * (q.y * q.y + q.z * q.z);
    const double r10 = 2 * (q.x * q.y + q.w * q.z);
    return std::atan2(r10, r00) * 180.0 / std::numbers::pi;
}

int main()
{
    const double dt = 0.01, rateDeg = 50.0;
    const double w = rateDeg * std::numbers::pi / 180.0;  // rad/s about body z
    Quat q{1, 0, 0, 0};
    std::cout << std::fixed << std::setprecision(4);
    std::cout << "    t   gyro-heading    shown-heading      q.w      q.z   |nose vector|\n";
    for (int k = 0; k <= 6000; ++k) {
        if (k % 600 == 0) {
            const double t = k * dt;
            double truth = std::fmod(rateDeg * t, 360.0);
            if (truth > 180.0) truth -= 360.0;
            const Vec3 nose = rotate(q, {1, 0, 0});
            std::cout << std::setw(5) << std::setprecision(0) << t << std::setprecision(4) << std::setw(15) << truth
                      << std::setw(17) << headingDeg(q) << std::setw(9) << q.w << std::setw(9) << q.z << std::setw(16)
                      << std::sqrt(nose.x * nose.x + nose.y * nose.y + nose.z * nose.z) << "\n";
        }
        const Quat dq = q * Quat{0, 0, 0, w};
        q = {q.w + 0.5 * dt * dq.w, q.x + 0.5 * dt * dq.x, q.y + 0.5 * dt * dq.y, q.z + 0.5 * dt * dq.z};
    }
    return 0;
}
