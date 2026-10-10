// loop_steps.cpp - step tests of each loop of the cascade (F10-16), innermost first.
// Each test starts from hover in the DN201 simulator and closes only the loops it needs;
// the controller sees the true state (a perfect estimator) so only the control is tested.
#include "cascade.hpp"

#include <cmath>
#include <cstdio>
#include <functional>
#include <vector>

namespace {

constexpr double kDeg = std::numbers::pi / 180.0;

struct Result
{
    double rise = -1.0;     // s, from 10 % to 90 % of the step
    double overshoot = 0.0; // % of the step
};

// Runs ms milliseconds; measure() returns the response; the step size is 'step'.
Result stepTest(dn301::Cascade& c, const std::function<double(const dn::State&)>& measure,
                double step, int ms, const dn301::Setpoint& sp = {})
{
    const dn::Params P;
    dn::State s;
    const double w0 = dn::hoverRotorSpeed(P);
    s.rotor = {w0, w0, w0, w0};
    double t10 = -1.0, t90 = -1.0, peak = 0.0;
    for (int k = 1; k <= ms; ++k) {
        const dn::Rotors cmd = c.step(s, sp, k);
        dn::rk4Step(P, s, cmd, 0.001);
        const double y = measure(s) / step;
        if (t10 < 0.0 && y >= 0.1) t10 = k * 1e-3;
        if (t90 < 0.0 && y >= 0.9) t90 = k * 1e-3;
        peak = std::max(peak, y);
    }
    Result r;
    if (t10 >= 0.0 && t90 >= 0.0) r.rise = t90 - t10;
    r.overshoot = std::max(0.0, (peak - 1.0) * 100.0);
    return r;
}

} // namespace

int main()
{
    const dn::Params P;
    std::printf("Step tests of the cascade, inner loop first (course quad, DN201 simulator)\n");
    std::printf("%-34s %10s %12s\n", "loop (step)", "rise (s)", "overshoot");

    { // 1. Rate loop alone: roll rate setpoint 1 rad/s, hover thrust.
        dn301::Cascade c(P);
        c.holdOuter = c.holdAttitude = true;
        c.trace.thrust = P.mass * P.g;
        c.trace.rateSp = {1.0, 0.0, 0.0};
        const Result r = stepTest(c, [](const dn::State& s) { return s.w.x; }, 1.0, 400);
        std::printf("%-34s %10.3f %10.1f %%\n", "1 rate (roll rate 1 rad/s)", r.rise, r.overshoot);
    }
    { // 2. Attitude + rate: roll 10 degrees, thrust that keeps the height.
        dn301::Cascade c(P);
        c.holdOuter = true;
        c.trace.attSp = {10.0 * kDeg, 0.0, 0.0};
        c.trace.thrust = P.mass * P.g / std::cos(10.0 * kDeg);
        const Result r = stepTest(
            c, [](const dn::State& s) { return dn::toEuler(s.q).roll; }, 10.0 * kDeg, 1500);
        std::printf("%-34s %10.3f %10.1f %%\n", "2 attitude (roll 10 deg)", r.rise, r.overshoot);
    }
    { // 3. Velocity + attitude + rate: forward speed 1 m/s.
        dn301::Cascade c(P);
        c.velocityOnly = true;
        c.trace.velSp = {1.0, 0.0, 0.0};
        const Result r = stepTest(c, [](const dn::State& s) { return s.v.x; }, 1.0, 5000);
        std::printf("%-34s %10.3f %10.1f %%\n", "3 velocity (vx 1 m/s)", r.rise, r.overshoot);
    }
    { // 4. The whole cascade: position x 2 m.
        dn301::Cascade c(P);
        dn301::Setpoint sp;
        sp.pos = {2.0, 0.0, 0.0};
        const Result r = stepTest(c, [](const dn::State& s) { return s.p.x; }, 2.0, 8000, sp);
        std::printf("%-34s %10.3f %10.1f %%\n", "4 position (x 2 m)", r.rise, r.overshoot);
    }

    // The position step again, logging every loop's setpoint against its actual value.
    std::printf("\nPosition step x = 2 m: setpoint versus actual at every level\n");
    std::printf(" t(s)  x_sp    x   | vx_sp   vx   | pitch_sp pitch (deg) | q_sp   q (rad/s)\n");
    dn301::Cascade c(P);
    dn301::Setpoint sp;
    sp.pos = {2.0, 0.0, 0.0};
    dn::State s;
    const double w0 = dn::hoverRotorSpeed(P);
    s.rotor = {w0, w0, w0, w0};
    for (int k = 1; k <= 4000; ++k) {
        const dn::Rotors cmd = c.step(s, sp, k);
        dn::rk4Step(P, s, cmd, 0.001);
        if (k % 200 == 0) {
            const auto& tr = c.trace;
            std::printf("%4.1f %5.2f %5.2f | %5.2f %5.2f | %7.2f %7.2f      | %5.2f %5.2f\n",
                        k * 1e-3, sp.pos.x, s.p.x, tr.velSp.x, s.v.x, tr.attSp.pitch / kDeg,
                        dn::toEuler(s.q).pitch / kDeg, tr.rateSp.y, s.w.y);
        }
    }
    return 0;
}
