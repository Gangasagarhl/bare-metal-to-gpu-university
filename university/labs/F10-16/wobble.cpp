// wobble.cpp - F10-16 forensic evidence generator "It will not stop swinging".
// The course quad, after a gain change, is told to move 1 m forward and hold position.
// One gain of the cascade differs from the course defaults; the answer key names it.
// The log has the setpoint and the actual value of every loop, as a flight log would.
#include "cascade.hpp"

#include <cmath>
#include <cstdio>

int main()
{
    const dn::Params P;
    dn301::Gains g;
    g.posP = 4.5; // the change under investigation (revealed in the answer key)
    std::printf("config: posP %.1f | velP %.1f velI %.1f | attP %.1f | rateP %.1f rateI %.1f "
                "rateD %.2f\n",
                g.posP, g.velP, g.velI, g.attP, g.rateP, g.rateI, g.rateD);
    dn301::Cascade c(P, g);
    dn301::Setpoint sp;
    sp.pos = {1.0, 0.0, 0.0};
    dn::State s;
    const double w0 = dn::hoverRotorSpeed(P);
    s.rotor = {w0, w0, w0, w0};
    constexpr double kDeg = std::numbers::pi / 180.0;
    std::printf(" t(s) | x_sp  x     | vx_sp  vx    | pitch_sp pitch | q_sp   q     | motors 1-4 "
                "(rad/s)\n");
    for (int k = 1; k <= 8000; ++k) {
        const dn::Rotors cmd = c.step(s, sp, k);
        dn::rk4Step(P, s, cmd, 0.001);
        if (k % 200 == 0) {
            const auto& tr = c.trace;
            std::printf("%5.1f | %4.2f %5.2f | %5.2f %5.2f | %7.1f %6.1f | %5.2f %5.2f | %4.0f "
                        "%4.0f %4.0f %4.0f\n",
                        k * 1e-3, sp.pos.x, s.p.x, tr.velSp.x, s.v.x, tr.attSp.pitch / kDeg,
                        dn::toEuler(s.q).pitch / kDeg, tr.rateSp.y, s.w.y, s.rotor[0], s.rotor[1],
                        s.rotor[2], s.rotor[3]);
        }
    }
    return 0;
}
