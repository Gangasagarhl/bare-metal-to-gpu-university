// F9-16 Listing 2: a brake is applied at t = 1.0 s. Open loop keeps sending the same
// voltage; closed loop measures the drop and pushes harder. Printed every 0.1 s.
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
    const double kp = 0.2;
    const double controlPeriod = 0.01;
    const int subSteps = 100;
    Motor open;
    Motor closed;
    double closedVolts = 0.0;
    std::cout << "  t (s)   brake (mN m)   open speed   closed speed   closed volts\n";
    for (int k = 0; k <= 200; ++k) {
        const double t = k * controlPeriod;
        const double brake = (t >= 1.0 - 1e-9) ? 0.002 : 0.0;
        const double openVolts = setpoint / open.kdc;
        closedVolts = std::clamp(kp * (setpoint - closed.w), -12.0, 12.0);
        if (k % 10 == 0) {
            std::cout << std::format("{:>7.2f} {:>14.1f} {:>12.1f} {:>14.1f} {:>14.3f}\n", t,
                                     brake * 1000.0, open.w, closed.w, closedVolts);
        }
        for (int s = 0; s < subSteps; ++s) {
            open.step(openVolts, brake, controlPeriod / subSteps);
            closed.step(closedVolts, brake, controlPeriod / subSteps);
        }
    }
    return 0;
}
