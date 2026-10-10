// windup.cpp - integrator windup before take-off (F10-19).
// The course quad stands on ground sloping 8 degrees (it rests rolled 8 degrees) for 4 s
// with motors at idle thrust while the attitude and rate loops already run with a level
// setpoint. The ground stops any rotation, so the rate error never goes away and the rate
// integrator charges. At 4.0 s the thrust rises to 1.3 times the weight and the vehicle
// lifts off. Two runs: integrators running on the ground, and integrators held at zero
// until lift-off (a "landed" rule; how real autopilots decide "landed" is not covered here).
#include "../F10-16/cascade.hpp"

#include <cmath>
#include <cstdio>

namespace {

constexpr double kDeg = std::numbers::pi / 180.0;

void run(bool holdIntegratorsOnGround)
{
    const dn::Params P;
    dn::State s;
    const dn::Quat slope = dn::fromEuler({8.0 * kDeg, 0.0, 0.0});
    s.q = slope;
    dn301::Cascade c(P);
    c.holdOuter = true;
    c.trace.attSp = {0.0, 0.0, 0.0};
    std::printf("%s\n", holdIntegratorsOnGround ? "Run B: integrators held at zero until lift-off"
                                                : "Run A: integrators running on the ground");
    std::printf("  t(s) | on ground | roll (deg) | p_sp  p (rad/s) | roll torque (N m)\n");
    double otherSide = 0.0; // most negative roll after lift-off: leaning the other way
    for (int k = 1; k <= 5500; ++k) {
        const bool ground = k <= 4000;
        c.trace.thrust = ground ? 0.3 * P.mass * P.g : 1.3 * P.mass * P.g;
        if (ground && holdIntegratorsOnGround) {
            c = dn301::Cascade(P); // fresh controller: every integrator at zero
            c.holdOuter = true;
            c.trace.attSp = {0.0, 0.0, 0.0};
            c.trace.thrust = 0.3 * P.mass * P.g;
        }
        const dn::Rotors cmd = c.step(s, {}, k);
        dn::rk4Step(P, s, cmd, 0.001);
        if (ground) { // the ground holds the vehicle still on its slope
            s.p = {};
            s.v = {};
            s.w = {};
            s.q = slope;
        }
        const double roll = dn::toEuler(s.q).roll / kDeg;
        if (!ground) otherSide = std::min(otherSide, roll);
        if (k % 500 == 0 || (k > 4000 && k <= 4600 && k % 100 == 0)) {
            std::printf("%6.2f | %9s | %10.2f | %5.2f %6.2f     | %8.4f\n", k * 1e-3,
                        ground ? "yes" : "no", roll, c.trace.rateSp.x, s.w.x, c.trace.torque.x);
        }
    }
    std::printf("  most negative roll after lift-off (leaning the other way): %.2f deg\n\n",
                otherSide);
}

} // namespace

int main()
{
    run(false);
    run(true);
    return 0;
}
