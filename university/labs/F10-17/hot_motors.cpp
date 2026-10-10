// hot_motors.cpp - F10-17 forensic evidence generator "The motors come down hot".
// The course quad holds level attitude in still air (attitude and rate loops on, outer
// loops off, hover thrust). The gyro has white noise of 0.02 rad/s. One tuning change made
// "to lock it in" is hidden in the gains below; the answer key explains it.
#include "../F10-16/cascade.hpp"
#include "../F10-13/imu.hpp"

#include <cmath>
#include <cstdio>

int main()
{
    const dn::Params P;
    dn301::Gains g;
    g.rateD = 1.2;    // the tuning change
    g.rateDTau = 0.0; // ... and the D-term filter switched off
    std::printf(
        "config: attP %.1f | rateP %.1f rateI %.1f rateD %.2f rateDTau %.3f s | loop 500 Hz\n",
        g.attP, g.rateP, g.rateI, g.rateD, g.rateDTau);
    dn301::Cascade c(P, g);
    c.holdOuter = true;
    c.trace.thrust = P.mass * P.g;
    c.trace.attSp = {0.0, 0.0, 0.0};
    dn::State s;
    const double w0 = dn::hoverRotorSpeed(P);
    s.rotor = {w0, w0, w0, w0};
    imu::Rng rng(31);

    std::printf("\nExcerpt, every rate-loop run (2 ms) from t = 1.000 s:\n");
    std::printf("   t(s)  | gyro x  | rate sp x | torque x (N m) | motor 1 cmd | motor 1 actual\n");
    double gyroSum = 0, gyroSq = 0, rollSq = 0, tqSum = 0, tqSq = 0, cmSum = 0, cmSq = 0, acSq = 0,
           acSum = 0;
    int n = 0;
    for (int k = 1; k <= 3000; ++k) {
        dn::State measured = s;
        measured.w.x += 0.02 * rng.gauss();
        measured.w.y += 0.02 * rng.gauss();
        measured.w.z += 0.02 * rng.gauss();
        const dn::Rotors cmd = c.step(measured, {}, k);
        dn::rk4Step(P, s, cmd, 0.001);
        if (k >= 1000 && k < 1024 && k % 2 == 0) {
            std::printf("  %6.3f | %7.4f | %9.4f | %14.5f | %11.1f | %14.1f\n", k * 1e-3,
                        measured.w.x, c.trace.rateSp.x, c.trace.torque.x, cmd[0], s.rotor[0]);
        }
        if (k > 1000 && k % 2 == 0) {
            gyroSum += measured.w.x;
            gyroSq += measured.w.x * measured.w.x;
            const double roll = dn::toEuler(s.q).roll;
            rollSq += roll * roll;
            tqSum += c.trace.torque.x;
            tqSq += c.trace.torque.x * c.trace.torque.x;
            cmSum += cmd[0];
            cmSq += cmd[0] * cmd[0];
            acSum += s.rotor[0];
            acSq += s.rotor[0] * s.rotor[0];
            ++n;
        }
    }
    auto sd = [n](double sum, double sq) {
        return std::sqrt(std::max(0.0, sq / n - (sum / n) * (sum / n)));
    };
    std::printf("\nStatistics from 1 s to 3 s (%d rate-loop runs):\n", n);
    std::printf("  gyro x std            %8.4f rad/s\n", sd(gyroSum, gyroSq));
    std::printf("  roll angle RMS        %8.4f deg\n", std::sqrt(rollSq / n) / imu::kDeg);
    std::printf("  torque x command std  %8.5f N m\n", sd(tqSum, tqSq));
    std::printf("  motor 1 command std   %8.2f rad/s (mean %.1f)\n", sd(cmSum, cmSq), cmSum / n);
    std::printf("  motor 1 actual std    %8.2f rad/s\n", sd(acSum, acSq));
    std::printf(
        "Pilot: 'Flies locked-in, but after two minutes the motors are too hot to touch.'\n");
    return 0;
}
