// F9-16 forensic evidence generator: "The fan that would not hold its speed".
// The fault is described only in the answer key. Columns: what the controller saw,
// what it commanded, and a reference tachometer clamped to the test rig.
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
    Motor m;
    double encoderReading = 0.0;
    std::cout << "  t (s)  setpoint  encoder speed  command (V)  tachometer speed  load (mN m)\n";
    for (int k = 0; k <= 400; ++k) {
        const double t = k * controlPeriod;
        const double load = (t >= 2.5 - 1e-9) ? 0.003 : 0.0;
        if (t < 1.5 - 1e-9) {
            encoderReading = m.w;
        }
        const double volts = std::clamp(kp * (setpoint - encoderReading), -12.0, 12.0);
        if (k % 25 == 0) {
            std::cout << std::format("{:>7.2f} {:>9.0f} {:>14.2f} {:>12.3f} {:>17.1f} {:>12.1f}\n",
                                     t, setpoint, encoderReading, volts, m.w, load * 1000.0);
        }
        for (int s = 0; s < subSteps; ++s) {
            m.step(volts, load, controlPeriod / subSteps);
        }
    }
    return 0;
}
