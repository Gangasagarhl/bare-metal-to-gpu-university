// F9-21 forensic evidence generator: "The motor that stopped listening".
// A first, hand-written PID (not pid.hpp) that measures the time between calls
// from a millisecond clock. The fault is described only in the answer key.
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
    const double ki = 0.25;
    const double kd = 0.0005;
    Motor m;
    double integral = 0.0;
    double previousError = 0.0;
    double dFiltered = 0.0;
    long previousMs = 0;
    long clockMs = 0;
    std::cout
        << " call  clock (ms)  dt (s)   speed   error   P term   I term    D term    command\n";
    for (int call = 1; call <= 160; ++call) {
        clockMs += (call == 151) ? 0 : 10; // the clock value read at each call
        const double dt = static_cast<double>(clockMs - previousMs) / 1000.0;
        previousMs = clockMs;
        const double error = 300.0 - m.w;
        integral += error * dt;
        const double rawD = (error - previousError) / dt;
        previousError = error;
        dFiltered += 0.2 * (rawD - dFiltered);
        const double command = std::clamp(kp * error + ki * integral + kd * dFiltered, -12.0, 12.0);
        // Simulated driver: a command that is not a number is treated as 0 V.
        const double volts = std::isfinite(command) ? command : 0.0;
        if (call >= 147) {
            std::cout << std::format(
                "{:>5} {:>11} {:>7.3f} {:>7.1f} {:>7.2f} {:>8.3f} {:>8.3f} {:>9.3f} {:>10.3f}\n",
                call, clockMs, dt, m.w, error, kp * error, ki * integral, kd * dFiltered, command);
        }
        for (int s = 0; s < 100; ++s) {
            m.step(volts, 0.0001);
        }
    }
    return 0;
}
