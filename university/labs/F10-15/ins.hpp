// ins.hpp - a GNSS-aided inertial navigation EKF (DN301, F10-15). 12 states:
//   x = [roll, pitch, yaw, vx, vy, vz, px, py, pz, bx, by, bz]  (rad, m/s, m, rad/s)
// Predict at the IMU rate: the gyro (minus bias) turns the attitude; the accelerometer,
// rotated into the world frame, plus gravity changes the velocity; velocity moves the position.
// Update at the GNSS rate with position and velocity, after an innovation gate.
// Axes as in DN201 (x forward, y left, z up; world z up). Exercise noise values.
#pragma once
#include "../F10-13/imu.hpp"
#include "mat.hpp"

#include <array>
#include <cmath>

namespace ins {

constexpr int N = 12;
using Vec = std::array<double, N>;

struct Inputs
{
    double gx, gy, gz, ax, ay, az; // gyro rad/s, accelerometer m/s^2 (body frame)
};

// Body-to-world rotation R = Rz(yaw) Ry(pitch) Rx(roll) applied to a body vector.
inline std::array<double, 3> toWorld(double r, double p, double y, double bx, double by, double bz)
{
    const double cr = std::cos(r), sr = std::sin(r), cp = std::cos(p), sp = std::sin(p);
    const double cy = std::cos(y), sy = std::sin(y);
    return {cy * cp * bx + (cy * sp * sr - sy * cr) * by + (cy * sp * cr + sy * sr) * bz,
            sy * cp * bx + (sy * sp * sr + cy * cr) * by + (sy * sp * cr - cy * sr) * bz,
            -sp * bx + cp * sr * by + cp * cr * bz};
}

inline Vec propagate(const Vec& x, const Inputs& u, double dt)
{
    const double p = u.gx - x[9], q = u.gy - x[10], w = u.gz - x[11];
    const double sr = std::sin(x[0]), cr = std::cos(x[0]);
    const double tp = std::tan(x[1]), cp = std::cos(x[1]);
    Vec n = x;
    n[0] += dt * (p + (q * sr + w * cr) * tp);
    n[1] += dt * (q * cr - w * sr);
    n[2] += dt * (q * sr + w * cr) / cp;
    const auto a = toWorld(x[0], x[1], x[2], u.ax, u.ay, u.az);
    n[3] += dt * a[0];
    n[4] += dt * a[1];
    n[5] += dt * (a[2] - imu::kG);
    n[6] += dt * x[3];
    n[7] += dt * x[4];
    n[8] += dt * x[5];
    return n;
}

struct Filter
{
    Vec x{};
    Mat P = Mat::identity(N);
    double gate = 5.0; // reject a GNSS sample if any innovation exceeds gate * sigma
    double gyroSigma = 0.003, accelSigma = 0.25, biasWalk = 2e-4;
    double posSigma = 0.5, velSigma = 0.1;
    long accepted = 0, rejected = 0;
    std::array<double, 6> lastInnovation{}, lastRatio{}; // ratio = |innovation| / sigma

    Filter()
    {
        const std::array<double, N> d{0.01, 0.01, 0.01, 0.01, 0.01, 0.01,
                                      0.25, 0.25, 0.25, 1e-4, 1e-4, 1e-4};
        for (int i = 0; i < N; ++i) {
            P(i, i) = d[static_cast<std::size_t>(i)];
        }
    }

    void predict(const Inputs& u, double dt)
    {
        // Numerical Jacobian of propagate() by central differences (12 x 12).
        Mat F(N, N);
        const double e = 1e-6;
        for (int j = 0; j < N; ++j) {
            Vec a = x, b = x;
            a[static_cast<std::size_t>(j)] += e;
            b[static_cast<std::size_t>(j)] -= e;
            const Vec fa = propagate(a, u, dt), fb = propagate(b, u, dt);
            for (int i = 0; i < N; ++i) {
                F(i, j) =
                    (fa[static_cast<std::size_t>(i)] - fb[static_cast<std::size_t>(i)]) / (2 * e);
            }
        }
        x = propagate(x, u, dt);
        Mat Q(N, N);
        const double qa = gyroSigma * gyroSigma * dt * dt, qv = accelSigma * accelSigma * dt * dt;
        const double qb = biasWalk * biasWalk * dt;
        for (int i = 0; i < 3; ++i) {
            Q(i, i) = qa;
            Q(i + 3, i + 3) = qv;
            Q(i + 9, i + 9) = qb;
        }
        P = F * P * F.t() + Q;
    }

    // z = measured [px, py, pz, vx, vy, vz]. Returns true if the sample was fused.
    bool updateGnss(const std::array<double, 6>& z)
    {
        Mat H(6, N);
        for (int i = 0; i < 3; ++i) {
            H(i, 6 + i) = 1.0;     // position
            H(3 + i, 3 + i) = 1.0; // velocity
        }
        Mat R(6, 6);
        for (int i = 0; i < 3; ++i) {
            R(i, i) = posSigma * posSigma;
            R(3 + i, 3 + i) = velSigma * velSigma;
        }
        const Mat S = H * P * H.t() + R;
        Mat y(6, 1);
        bool pass = true;
        for (int i = 0; i < 6; ++i) {
            const std::size_t k = static_cast<std::size_t>(i);
            const double predicted = i < 3 ? x[6 + k] : x[k];
            y(i, 0) = z[k] - predicted;
            lastInnovation[k] = y(i, 0);
            lastRatio[k] = std::fabs(y(i, 0)) / std::sqrt(S(i, i));
            pass = pass && lastRatio[k] <= gate;
        }
        if (!pass) {
            ++rejected;
            return false;
        }
        const Mat K = P * H.t() * inverse(S);
        const Mat dx = K * y;
        for (int i = 0; i < N; ++i) {
            x[static_cast<std::size_t>(i)] += dx(i, 0);
        }
        const Mat IKH = Mat::identity(N) - K * H;
        P = IKH * P * IKH.t() + K * R * K.t();
        ++accepted;
        return true;
    }
};

} // namespace ins
