// square_mission.cpp - the cascade of F10-16 flies a 4 m square in the DN201 simulator,
// first with the true state, then with attitude from an angle-blending complementary
// filter (F10-14) fed by a modelled IMU (gyro bias and noise, accelerometer noise).
// Position and velocity stay true in both runs, so the difference is the attitude estimate.
#include "../F10-13/imu.hpp"
#include "cascade.hpp"

#include <array>
#include <cmath>
#include <cstdio>

namespace {

constexpr double kDeg = std::numbers::pi / 180.0;

struct Outcome
{
    double posRms = 0.0, maxTiltErr = 0.0, finalErr = 0.0;
};

Outcome fly(bool useFilter, double tau)
{
    const dn::Params P;
    dn301::Cascade c(P);
    dn::State s;
    const double w0 = dn::hoverRotorSpeed(P);
    s.rotor = {w0, w0, w0, w0};
    const std::array<dn::Vec3, 5> corner{{{0, 0, 0}, {4, 0, 0}, {4, 4, 0}, {0, 4, 0}, {0, 0, 0}}};
    imu::Rng rng(2026);
    double roll = 0.0, pitch = 0.0; // filter state
    Outcome o;
    double sq = 0.0;
    int n = 0;
    for (int k = 1; k <= 20000; ++k) {
        dn301::Setpoint sp;
        sp.pos = corner[static_cast<std::size_t>(std::min(k / 4000, 4))];
        dn::State est = s;
        if (useFilter) {
            // The IMU, sampled every millisecond: true rates and specific force, plus errors.
            const dn::Deriv d = dn::derivative(P, s, c.command());
            const dn::Quat qc{s.q.w, -s.q.x, -s.q.y, -s.q.z};
            const dn::Vec3 f = dn::rotate(qc, d.dv + dn::Vec3{0.0, 0.0, P.g});
            const double gx = s.w.x + 0.004 + 0.01 * rng.gauss();
            const double gy = s.w.y - 0.006 + 0.01 * rng.gauss();
            const double gz = s.w.z + 0.003 + 0.01 * rng.gauss();
            const double dt = 0.001;
            const double sr = std::sin(roll), cr = std::cos(roll);
            roll += (gx + (gy * sr + gz * cr) * std::tan(pitch)) * dt;
            pitch += (gy * cr - gz * sr) * dt;
            const imu::Tilt a = imu::accelTilt(f.x + 0.25 * rng.gauss(), f.y + 0.25 * rng.gauss(),
                                               f.z + 0.25 * rng.gauss());
            const double alpha = tau / (tau + dt);
            roll = alpha * roll + (1.0 - alpha) * a.roll;
            pitch = alpha * pitch + (1.0 - alpha) * a.pitch;
            const dn::Euler truth = dn::toEuler(s.q);
            est.q = dn::fromEuler({roll, pitch, truth.yaw}); // yaw from elsewhere (true here)
            est.w = {gx, gy, gz};
            o.maxTiltErr = std::max(
                {o.maxTiltErr, std::abs(roll - truth.roll), std::abs(pitch - truth.pitch)});
        }
        dn::rk4Step(P, s, c.step(est, sp, k), 0.001);
        const dn::Vec3 e = sp.pos - s.p;
        sq += dn::dot(e, e);
        ++n;
    }
    o.posRms = std::sqrt(sq / n);
    o.finalErr = std::sqrt(dn::dot(s.p, s.p));
    o.maxTiltErr /= kDeg;
    return o;
}

} // namespace

int main()
{
    std::printf(
        "4 m square, 4 s per side, then hold (20 s). Position error = setpoint - position.\n");
    std::printf("%-30s %14s %16s %16s\n", "attitude source", "pos RMS (m)", "max tilt err",
                "final err (m)");
    const Outcome t = fly(false, 0.0);
    std::printf("%-30s %14.3f %12s     %16.3f\n", "true state", t.posRms, "-", t.finalErr);
    for (double tau : {0.3, 1.0, 3.0}) {
        const Outcome o = fly(true, tau);
        char name[40];
        std::snprintf(name, sizeof name, "complementary filter tau %.1f s", tau);
        std::printf("%-30s %14.3f %12.2f deg %16.3f\n", name, o.posRms, o.maxTiltErr, o.finalErr);
    }
    return 0;
}
