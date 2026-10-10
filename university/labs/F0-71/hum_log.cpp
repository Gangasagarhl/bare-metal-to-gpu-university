// F0-71 forensic evidence generator ("the humming motor"): after a tuning change the
// speed loop of Listing 2 hums. The driver limits the command to +/- 12 V. The speed is
// logged every 5 ms once the setpoint of 100 rad/s has been held for 2 s. The gain used
// is given only in the answer key.
#include <algorithm>
#include <cmath>
#include <format>
#include <iostream>

int main()
{
    const double tm = 0.19608;
    const double ta = 0.01;
    const double tf = 0.02;
    const double kdc = 98.039;
    const double kp = 0.5;
    const double dt = 1e-5;
    double u = 0.0;
    double w = 0.0;
    double y = 0.0;
    std::cout << "# t (s), speed (rad/s), command (V)\n";
    const int n = static_cast<int>(std::lround(2.2 / dt));
    for (int k = 0; k <= n; ++k) {
        const double t = k * dt;
        const double command = std::clamp(kp * (100.0 - y), -12.0, 12.0);
        if (k >= 200000 && k % 500 == 0) {
            std::cout << std::format("{:.3f} {:>7.1f} {:>6.2f}\n", t, w, command);
        }
        u += (command - u) / ta * dt;
        w += (kdc * u - w) / tm * dt;
        y += (w - y) / tf * dt;
    }
    return 0;
}
