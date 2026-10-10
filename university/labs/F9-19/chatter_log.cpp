// F9-19 forensic evidence generator: "The cart that started to chatter".
// Two logs of the same move (setpoint 0 -> 1 m), before and after a change.
// The position sensor reports whole millimetres. Rows from t = 0.300 s.
#include <algorithm>
#include <cmath>
#include <format>
#include <iostream>

struct Cart
{
    double mass = 2.0;
    double friction = 0.8;
    double x = 0.0;
    double v = 0.0;

    void step(double force, double dt)
    {
        v += dt * (force - friction * v) / mass;
        x += dt * v;
    }
};

void run(double period, int rows)
{
    const double kp = 8.0;
    const double kd = 4.8;
    const int subSteps = static_cast<int>(std::lround(period / 0.00001));
    Cart cart;
    double previousReading = 0.0;
    std::cout << std::format("log: control period {:.3f} s\n", period);
    std::cout << "   t (s)  sensor (mm)  P term (N)  D term (N)  force (N)\n";
    const int first = static_cast<int>(std::lround(0.3 / period));
    for (int k = 0; k < first + rows; ++k) {
        const double reading = std::round(cart.x * 1000.0) / 1000.0; // whole millimetres
        const double pTerm = kp * (1.0 - reading);
        const double dTerm = -kd * (reading - previousReading) / period;
        previousReading = reading;
        const double force = std::clamp(pTerm + dTerm, -10.0, 10.0);
        if (k >= first) {
            std::cout << std::format("{:>8.3f} {:>12.0f} {:>11.3f} {:>11.3f} {:>10.3f}\n",
                                     k * period, reading * 1000.0, pTerm, dTerm, force);
        }
        for (int s = 0; s < subSteps; ++s) {
            cart.step(force, period / subSteps);
        }
    }
    std::cout << "\n";
}

int main()
{
    run(0.01, 6);
    run(0.001, 24);
    return 0;
}
