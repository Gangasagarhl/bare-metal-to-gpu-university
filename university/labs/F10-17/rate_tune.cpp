// rate_tune.cpp - tuning the roll-rate PID of the cascade (F10-16) one change at a time.
// Rate loop alone (outer loops off), 500 Hz, DN201 simulator; the gyro reading has white
// noise (0.02 rad/s). Test: roll rate +1 rad/s from 0.1 s to 0.6 s (step response), -1 rad/s
// to 1.1 s (back to level), then 0. At 1.2 s motor 1's propeller becomes 5 % stronger than
// the mixer assumes: a constant roll-torque disturbance the loop must reject.
#include "../F10-16/cascade.hpp"
#include "../F10-13/imu.hpp"

#include <cmath>
#include <cstdio>

namespace {

struct Trial
{
    const char* change;
    double kp, ki, kd, dTau;
};

struct Metrics
{
    double rise = -1.0, overshoot = 0.0, steadyErr = 0.0, motorNoise = 0.0, peakRoll = 0.0;
};

Metrics run(const Trial& tr)
{
    dn::Params P;
    dn301::Gains g;
    g.rateP = tr.kp;
    g.rateI = tr.ki;
    g.rateD = tr.kd;
    g.rateDTau = tr.dTau;
    dn301::Cascade c(P, g);
    c.holdOuter = c.holdAttitude = true;
    c.trace.thrust = P.mass * P.g;
    dn::State s;
    const double w0 = dn::hoverRotorSpeed(P);
    s.rotor = {w0, w0, w0, w0};
    imu::Rng rng(7);
    Metrics m;
    double t10 = -1.0, t90 = -1.0, peak = 0.0, errSum = 0.0, cmdSum = 0.0, cmdSq = 0.0;
    int errN = 0, cmdN = 0;
    for (int k = 1; k <= 2000; ++k) {
        const double t = k * 1e-3;
        if (k == 1200) {
            P.kTscale = {1.05, 1.0, 1.0, 1.0}; // the disturbance starts
        }
        c.trace.rateSp = {t < 0.1 ? 0.0 : t < 0.6 ? 1.0 : t < 1.1 ? -1.0 : 0.0, 0.0, 0.0};
        dn::State measured = s;
        measured.w.x += 0.02 * rng.gauss(); // gyro noise
        const dn::Rotors cmd = c.step(measured, {}, k);
        dn::rk4Step(P, s, cmd, 0.001);
        if (t > 0.1 && t <= 0.6) {
            const double y = s.w.x;
            if (t10 < 0.0 && y >= 0.1) t10 = t;
            if (t90 < 0.0 && y >= 0.9) t90 = t;
            peak = std::max(peak, y);
        }
        if (t > 1.6) { // disturbance rejection: mean rate error after 0.4 s
            errSum += 0.0 - s.w.x;
            ++errN;
        }
        if (t > 1.6 && k % 2 == 0) { // spread of motor 1's command, one per rate-loop run
            cmdSum += cmd[0];
            cmdSq += cmd[0] * cmd[0];
            ++cmdN;
        }
        m.peakRoll = std::max(m.peakRoll, std::abs(dn::toEuler(s.q).roll));
    }
    if (t10 >= 0.0 && t90 >= 0.0) m.rise = t90 - t10;
    m.overshoot = std::max(0.0, (peak - 1.0) * 100.0);
    m.steadyErr = errSum / errN;
    const double mean = cmdSum / cmdN;
    m.motorNoise = std::sqrt(std::max(0.0, cmdSq / cmdN - mean * mean));
    m.peakRoll /= imu::kDeg;
    return m;
}

} // namespace

int main()
{
    const Trial trials[] = {
        {"1 P only, low", 5.0, 0.0, 0.0, 0.005},
        {"2 raise P", 10.0, 0.0, 0.0, 0.005},
        {"3 raise P again", 20.0, 0.0, 0.0, 0.005},
        {"4 too much P", 60.0, 0.0, 0.0, 0.005},
        {"5 back to P 20, add D", 20.0, 0.0, 0.3, 0.005},
        {"6 same, D filter off", 20.0, 0.0, 0.3, 0.0},
        {"7 D filter on, add I", 20.0, 5.0, 0.3, 0.005},
        {"8 more I", 20.0, 40.0, 0.3, 0.005},
    };
    std::printf(
        "Roll-rate tuning, one change at a time (step 1 rad/s; 5 %% disturbance at 1.2 s)\n");
    std::printf("%-24s %5s %5s %4s %6s | %8s %9s %10s %10s\n", "trial", "P", "I", "D", "Dtau",
                "rise(s)", "overshoot", "dist. err", "motor1 std");
    for (const Trial& t : trials) {
        const Metrics m = run(t);
        std::printf("%-24s %5.1f %5.1f %4.2f %6.3f | %8.3f %8.1f%% %10.3f %10.2f\n", t.change, t.kp,
                    t.ki, t.kd, t.dTau, m.rise, m.overshoot, m.steadyErr, m.motorNoise);
    }
    std::printf("dist. err: mean of (0 - roll rate) from 1.6 s to 2.0 s, rad/s;\n");
    std::printf(
        "motor1 std: standard deviation of motor 1's speed command, 1.6 s to 2.0 s, rad/s\n");
    return 0;
}
