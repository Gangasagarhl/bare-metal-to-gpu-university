// yaw_compare.cpp - the yaw manoeuvre of yaw_jump.cpp (heading step of 120 degrees while
// holding position) flown with three mixers.
#include "../F10-16/cascade.hpp"
#include "allocation.hpp"

#include <cmath>
#include <cstdio>

namespace {

constexpr double kDeg = std::numbers::pi / 180.0;

struct Summary
{
    double maxZ = 0.0, maxTilt = 0.0, yawRise = -1.0, yawPeak = 0.0;
};

Summary yawStep(dn301::Cascade::Allocator a)
{
    const dn::Params P;
    dn301::Cascade c(P);
    c.allocator = a;
    dn301::Setpoint sp;
    dn::State s;
    const double w0 = dn::hoverRotorSpeed(P);
    s.rotor = {w0, w0, w0, w0};
    Summary m;
    for (int k = 1; k <= 5000; ++k) {
        if (k == 1000) sp.yaw = 120.0 * kDeg;
        dn::rk4Step(P, s, c.step(s, sp, k), 0.001);
        const dn::Euler e = dn::toEuler(s.q);
        m.maxZ = std::max(m.maxZ, std::abs(s.p.z));
        m.maxTilt = std::max({m.maxTilt, std::abs(e.roll), std::abs(e.pitch)});
        if (m.yawRise < 0.0 && e.yaw >= 0.9 * sp.yaw && k >= 1000) m.yawRise = k * 1e-3 - 1.0;
        m.yawPeak = std::max(m.yawPeak, e.yaw);
    }
    return m;
}

} // namespace

int main()
{
    struct Mixer
    {
        const char* name;
        dn301::Cascade::Allocator a;
    };
    const Mixer mixers[] = {{"naive clip (DN201 mix)", &dn::mix},
                            {"RP > T > Y", &alloc::thrustBeforeYaw},
                            {"RP > Y > T", &alloc::yawBeforeThrust}};
    std::printf("Heading step 0 -> 120 deg at 1.0 s, holding position (5 s simulated)\n");
    std::printf("%-24s %11s %12s %14s %12s\n", "mixer", "max |z| (m)", "max tilt", "yaw 90% (s)",
                "yaw peak");
    for (const Mixer& m : mixers) {
        const Summary r = yawStep(m.a);
        std::printf("%-24s %11.3f %9.2f deg %14.3f %8.1f deg\n", m.name, r.maxZ, r.maxTilt / kDeg,
                    r.yawRise, r.yawPeak / kDeg);
    }
    return 0;
}
