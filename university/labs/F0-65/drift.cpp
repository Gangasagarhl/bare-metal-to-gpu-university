// F0-65 forensic evidence generator: dead reckoning of a cart that drives 2 m and
// then parks against a wall. The speed sensor has a small fault (described only in
// the answer key); the program integrates the measured speed to estimate position,
// as the robot's software did.
#include <format>
#include <iostream>

int main()
{
    const double dt = 0.01;          // 100 samples per second
    const double offset = 0.004;     // m/s, the fault
    double position = 0.0;
    std::cout << "robot log: drives for 5 s, then parked against the wall (the tape measure says 2.00 m)\n";
    std::cout << " t (s)   measured speed (m/s)   estimated position (m)\n";
    const int steps = 60500;         // 5 s of driving and 600 s parked
    for (int k = 1; k <= steps; ++k) {
        const double t = k * dt;
        const double trueSpeed = (t <= 5.0) ? 0.4 : 0.0;
        const double measured = trueSpeed + offset;
        position += measured * dt;
        if (k == 100 || k == 500 || k == 1500 || k == 6500 || k == 12500 || k == 30500 || k == 60500) {
            std::cout << std::format("{:>6.0f} {:>22.3f} {:>24.3f}\n", t, measured, position);
        }
    }
    return 0;
}
