// F0-70 forensic evidence generator: speed log of a motor whose speed controller was
// "refactored". The driver limits the command to +/- 12 V. The fault is described only
// in the answer key.
#include <algorithm>
#include <cmath>
#include <format>
#include <iostream>

double controller(double setpoint, double measured)
{
    const double kp = 0.05;
    const double error = measured - setpoint;
    return std::clamp(kp * error, -12.0, 12.0);
}

int main()
{
    const double r = 2.0;
    const double l = 1e-3;
    const double k = 0.01;
    const double j = 1e-5;
    const double b = 1e-6;
    const double dt = 1e-6;
    double i = 0.0;
    double w = 0.0;
    std::cout << "# columns: t (s), setpoint (rad/s), measured speed (rad/s), command (V)\n";
    const int n = static_cast<int>(std::lround(2.0 / dt));
    for (int s = 0; s <= n; ++s) {
        const double t = s * dt;
        const double setpoint = (t < 1.0) ? 100.0 : -300.0;
        const double volts = controller(setpoint, w);
        if (s % 100000 == 0) {
            std::cout << std::format("{:.1f} {:>6.0f} {:>9.1f} {:>7.2f}\n", t, setpoint, w, volts);
        }
        const double di = (volts - r * i - k * w) / l;
        const double dw = (k * i - b * w) / j;
        i += di * dt;
        w += dw * dt;
    }
    return 0;
}
