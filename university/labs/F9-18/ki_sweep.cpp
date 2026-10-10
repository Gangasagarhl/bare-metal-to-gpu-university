// F9-18 Listing 2: how the integral gain shapes a small step (0 -> 100 rad/s, small
// enough that the 12 V limit is never reached). Kp = 0.05 V per rad/s throughout.
#include <algorithm>
#include <cmath>
#include <format>
#include <iostream>

struct Motor
{
    double tau = 1e-5 * 2.0 / (0.01 * 0.01 + 2.0 * 1e-6);
    double kdc = 0.01 / (0.01 * 0.01 + 2.0 * 1e-6);
    double w = 0.0;

    void step(double volts, double dt) { w += dt * (kdc * volts - w) / tau; }
};

int main()
{
    const double kp = 0.05;
    const double setpoint = 100.0;
    const double period = 0.01;
    std::cout << "    Ki    overshoot   settling (2 %)   error at 3 s   largest command\n";
    for (double ki : {0.0, 0.1, 0.25, 1.0, 3.0}) {
        Motor m;
        double integral = 0.0;
        double peak = 0.0;
        double settle = 0.0;
        double maxVolts = 0.0;
        for (int k = 0; k < 300; ++k) {
            const double error = setpoint - m.w;
            integral += error * period;
            const double volts = std::clamp(kp * error + ki * integral, -12.0, 12.0);
            maxVolts = std::max(maxVolts, std::abs(volts));
            for (int s = 0; s < 100; ++s) {
                m.step(volts, period / 100);
            }
            peak = std::max(peak, m.w);
            if (std::abs(setpoint - m.w) > 0.02 * setpoint) {
                settle = (k + 1) * period;
            }
        }
        std::cout << std::format("{:>6.2f} {:>10.1f} % {:>14.2f} s {:>12.2f} {:>15.2f} V\n", ki,
                                 std::max(0.0, 100.0 * (peak - setpoint) / setpoint), settle,
                                 setpoint - m.w, maxVolts);
    }
    return 0;
}
