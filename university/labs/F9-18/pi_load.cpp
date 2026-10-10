// F9-18 Listing 1: P control versus PI control of the pretend motor's speed.
// Setpoint 300 rad/s from t = 0; a brake of 2 mN m is applied at t = 1.5 s.
// The PI controller adds a running sum of the error (the integral term).
#include <algorithm>
#include <format>
#include <iostream>

struct Motor
{
    double tau = 1e-5 * 2.0 / (0.01 * 0.01 + 2.0 * 1e-6);
    double kdc = 0.01 / (0.01 * 0.01 + 2.0 * 1e-6);
    double kload = 2.0 / (0.01 * 0.01 + 2.0 * 1e-6);
    double w = 0.0;

    void step(double volts, double loadTorque, double dt)
    {
        w += dt * (kdc * volts - kload * loadTorque - w) / tau;
    }
};

int main()
{
    const double setpoint = 300.0;
    const double kp = 0.05; // V per (rad/s)
    const double ki = 0.25; // V per rad, i.e. per (rad/s of error) per second
    const double period = 0.01;
    Motor pOnly;
    Motor pi;
    double integral = 0.0; // running sum of error * period, in rad
    std::cout << "  t (s)  brake   P-only speed   PI speed   PI: P term   I term   command (V)\n";
    for (int k = 0; k <= 300; ++k) {
        const double t = k * period;
        const double brake = (t >= 1.5 - 1e-9) ? 0.002 : 0.0;
        const double vP = std::clamp(kp * (setpoint - pOnly.w), -12.0, 12.0);
        const double error = setpoint - pi.w;
        integral += error * period;
        const double pTerm = kp * error;
        const double iTerm = ki * integral;
        const double vPI = std::clamp(pTerm + iTerm, -12.0, 12.0);
        if (k % 25 == 0) {
            std::cout << std::format(
                "{:>7.2f} {:>6.1f} {:>14.1f} {:>10.1f} {:>12.3f} {:>8.3f} {:>13.3f}\n", t,
                brake * 1000.0, pOnly.w, pi.w, pTerm, iTerm, vPI);
        }
        for (int s = 0; s < 100; ++s) {
            pOnly.step(vP, brake, period / 100);
            pi.step(vPI, brake, period / 100);
        }
    }
    return 0;
}
