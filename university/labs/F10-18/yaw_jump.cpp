// yaw_jump.cpp - F10-18 forensic evidence generator "It jumps when it turns".
// The course quad holds position at the origin with the full cascade of F10-16 and the
// mixer shown in its config line. At 1.0 s the pilot asks for a heading of 120 degrees.
#include "../F10-16/cascade.hpp"

#include <cmath>
#include <cstdio>

int main()
{
    const dn::Params P;
    dn301::Cascade c(P);
    c.allocator = &dn::mix; // the vehicle's mixer (DN201 F10-02 / quadsim.hpp mix)
    std::printf("config: mixer = DN201 mix (exact inverse, each rotor clipped to [0, max]); "
                "yawP %.1f yawRateP %.1f\n",
                c.gains().yawP, c.gains().yawRateP);
    dn301::Setpoint sp;
    dn::State s;
    const double w0 = dn::hoverRotorSpeed(P);
    s.rotor = {w0, w0, w0, w0};
    constexpr double kDeg = std::numbers::pi / 180.0;
    std::printf(" t(s) | yaw_sp  yaw   | r_sp   r     | z      | thrust_sp | tz_sp  | motors 1-4 "
                "(rad/s)\n");
    for (int k = 1; k <= 3500; ++k) {
        if (k == 1000) {
            sp.yaw = 120.0 * kDeg;
        }
        const dn::Rotors cmd = c.step(s, sp, k);
        dn::rk4Step(P, s, cmd, 0.001);
        if (k >= 800 && k % 100 == 0) {
            const auto& tr = c.trace;
            std::printf("%5.1f | %6.1f %6.1f | %5.2f %5.2f | %6.3f | %9.2f | %6.3f | %4.0f %4.0f "
                        "%4.0f %4.0f\n",
                        k * 1e-3, sp.yaw / kDeg, dn::toEuler(s.q).yaw / kDeg, tr.rateSp.z, s.w.z,
                        s.p.z, tr.thrust, tr.torque.z, cmd[0], cmd[1], cmd[2], cmd[3]);
        }
    }
    return 0;
}
