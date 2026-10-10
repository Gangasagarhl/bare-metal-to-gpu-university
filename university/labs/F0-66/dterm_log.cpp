// F0-66 forensic evidence generator: the derivative term of a motor position
// controller, logged at 1 kHz. The fault is described only in the answer key.
#include <cmath>
#include <format>
#include <iostream>

int main()
{
    const double dt = 0.001;
    const double countsPerRad = 4096.0 / (2.0 * 3.141592653589793);
    const double kd = 0.05;             // V per (rad/s)
    double prevAngle = 0.0;
    std::cout << "  t (ms)  encoder counts  angle (rad)  D term (V)\n";
    for (int k = 0; k <= 40; ++k) {
        const double t = k * dt;
        const double trueAngle = 0.3 * t;   // slow, steady turn: 0.3 rad/s
        const long counts = std::lround(trueAngle * countsPerRad);
        const double angle = static_cast<double>(counts) / countsPerRad;
        const double dTerm = (k == 0) ? 0.0 : kd * (angle - prevAngle) / dt;
        prevAngle = angle;
        if (k >= 20) {
            std::cout << std::format("{:>8} {:>15} {:>12.6f} {:>11.3f}\n", k, counts, angle, dTerm);
        }
    }
    return 0;
}
