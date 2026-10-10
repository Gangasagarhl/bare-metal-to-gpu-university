// F9-18 forensic evidence generator (course forensic lab "Windup").
// The same PI gains are given two setpoint steps: a small one and a large one.
// The driver can deliver at most 12 V. Logged every 50 ms.
#include <algorithm>
#include <format>
#include <iostream>

struct Motor
{
    double tau = 1e-5 * 2.0 / (0.01 * 0.01 + 2.0 * 1e-6);
    double kdc = 0.01 / (0.01 * 0.01 + 2.0 * 1e-6);
    double w = 0.0;

    void step(double volts, double dt) { w += dt * (kdc * volts - w) / tau; }
};

void run(double setpoint, int lastSample)
{
    const double kp = 0.05;
    const double ki = 0.25;
    const double period = 0.01;
    Motor m;
    double integral = 0.0;
    double peak = 0.0;
    std::cout << std::format("run: setpoint step 0 -> {:.0f} rad/s at t = 0\n", setpoint);
    std::cout << "  t (s)   speed   error   P term   I term   asked (V)  applied (V)\n";
    for (int k = 0; k <= lastSample; ++k) {
        const double error = setpoint - m.w;
        integral += error * period;
        const double pTerm = kp * error;
        const double iTerm = ki * integral;
        const double asked = pTerm + iTerm;
        const double applied = std::clamp(asked, -12.0, 12.0);
        if (k % 5 == 0) {
            std::cout << std::format(
                "{:>7.2f} {:>7.1f} {:>7.1f} {:>8.2f} {:>8.2f} {:>11.2f} {:>12.2f}\n", k * period,
                m.w, error, pTerm, iTerm, asked, applied);
        }
        for (int s = 0; s < 100; ++s) {
            m.step(applied, period / 100);
        }
        peak = std::max(peak, m.w);
    }
    std::cout << std::format("peak speed {:.1f} rad/s = {:+.1f} % compared with the setpoint\n\n",
                             peak, 100.0 * (peak - setpoint) / setpoint);
}

int main()
{
    run(100.0, 40);
    run(900.0, 150);
    return 0;
}
