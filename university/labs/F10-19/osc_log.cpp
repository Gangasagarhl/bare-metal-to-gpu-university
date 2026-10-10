// osc_log.cpp - F10-19 forensic evidence generator "Oscillation" (the DN301 course forensic).
// The course quad hovers with attitude hold (outer loops off) after a tuning session that
// changed two settings; at 1.0 s the pilot gives a short roll input (5 degrees for 0.3 s),
// then centres the stick. The vehicle model includes 6 ms of sensing and actuation delay
// (gyro filtering and ESC response lumped together, an exercise value) and gyro noise of
// 0.02 rad/s.
// The log has setpoint and actual values at 250 Hz, as a flight log's controller topics would.
#include "../F10-13/imu.hpp"
#include "../F10-16/cascade.hpp"

#include <cmath>
#include <cstdio>
#include <deque>

int main()
{
    const dn::Params P;
    dn301::Gains g;         // course defaults, except:
    g.attP = 9.0;           // change 1 of the tuning session
    g.rollRatePScale = 6.0; // change 2: roll rate P = 6 x 20 = 120
    std::printf("course defaults: attP 6.0 | rateP 20.0 (roll and pitch) rateI 5.0 rateD 0.30\n");
    std::printf("this flight:     attP %.1f | roll rateP %.1f, pitch rateP %.1f, rateI %.1f "
                "rateD %.2f\n",
                g.attP, g.rateP * g.rollRatePScale, g.rateP, g.rateI, g.rateD);
    dn301::Cascade c(P, g);
    c.holdOuter = true;
    c.trace.thrust = P.mass * P.g;
    dn::State s;
    const double w0 = dn::hoverRotorSpeed(P);
    s.rotor = {w0, w0, w0, w0};
    std::deque<dn::State> delayLine; // the controller sees the state 6 ms late
    imu::Rng rng(5);
    constexpr double kDeg = std::numbers::pi / 180.0;
    std::printf("\n t(s)  | roll_sp roll  | p_sp   p     | pitch_sp pitch | q_sp   q     | m1   m2 "
                "  m3   m4\n");
    double sumP = 0.0, sumQ = 0.0, sumP0 = 0.0, sumQ0 = 0.0;
    int n = 0, n0 = 0;
    for (int k = 1; k <= 2000; ++k) {
        const double t = k * 1e-3;
        c.trace.attSp = {(t >= 1.0 && t < 1.3) ? 5.0 * kDeg : 0.0, 0.0, 0.0};
        delayLine.push_back(s);
        dn::State seen = delayLine.front();
        seen.w = seen.w + dn::Vec3{0.02 * rng.gauss(), 0.02 * rng.gauss(), 0.02 * rng.gauss()};
        if (delayLine.size() > 6) delayLine.pop_front();
        const dn::Rotors cmd = c.step(seen, {}, k);
        dn::rk4Step(P, s, cmd, 0.001);
        const bool window = (k >= 700 && k < 800) || (k >= 1500 && k < 1600);
        if (window && k % 4 == 0) {
            const dn::Euler e = dn::toEuler(s.q);
            std::printf("%6.3f | %6.2f %6.2f | %5.2f %5.2f | %7.2f %6.2f | %5.2f %5.2f | %4.0f "
                        "%4.0f %4.0f %4.0f\n",
                        t, c.trace.attSp.roll / kDeg, e.roll / kDeg, c.trace.rateSp.x, s.w.x,
                        c.trace.attSp.pitch / kDeg, e.pitch / kDeg, c.trace.rateSp.y, s.w.y, cmd[0],
                        cmd[1], cmd[2], cmd[3]);
            if (k == 796)
                std::printf("  ...  (0.800 s to 1.500 s not shown; roll input 1.0-1.3 s)\n");
        }
        if (t > 0.5 && t <= 1.0) {
            sumP0 += s.w.x * s.w.x;
            sumQ0 += s.w.y * s.w.y;
            ++n0;
        }
        if (t > 1.5) {
            sumP += s.w.x * s.w.x;
            sumQ += s.w.y * s.w.y;
            ++n;
        }
    }
    std::printf(
        "\nRMS body rates 0.5 s to 1.0 s: roll rate p %.3f rad/s, pitch rate q %.3f rad/s\n",
        std::sqrt(sumP0 / n0), std::sqrt(sumQ0 / n0));
    std::printf("RMS body rates 1.5 s to 2.0 s: roll rate p %.3f rad/s, pitch rate q %.3f rad/s\n",
                std::sqrt(sumP / n), std::sqrt(sumQ / n));
    std::printf(
        "Pilot: 'Since the tuning session it buzzes in hover, worse after any roll input.'\n");
    return 0;
}
