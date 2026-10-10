// F9-19 Listing 3: two ways to compute the derivative term, at the moment of a
// setpoint step (0 -> 1 m at t = 0.5 s). Derivative of the ERROR jumps when the
// setpoint jumps ("derivative kick"); derivative of the MEASUREMENT does not.
#include <algorithm>
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

int main()
{
    const double kp = 8.0;
    const double kd = 4.8;
    const double period = 0.01;
    for (bool onError : {true, false}) {
        Cart cart;
        double previousError = 0.0;
        double previousX = 0.0;
        std::cout << (onError ? "derivative of the error:\n" : "derivative of the measurement:\n");
        std::cout << "  t (s)  setpoint   x (m)    P term   D term   asked (N)  applied (N)\n";
        for (int k = 0; k <= 56; ++k) {
            const double t = k * period;
            const double setpoint = (t >= 0.5 - 1e-9) ? 1.0 : 0.0;
            const double error = setpoint - cart.x;
            const double dTerm = onError ? kd * (error - previousError) / period
                                         : -kd * (cart.x - previousX) / period;
            previousError = error;
            previousX = cart.x;
            const double pTerm = kp * error;
            const double asked = pTerm + dTerm;
            const double applied = std::clamp(asked, -10.0, 10.0);
            if (k >= 49) {
                std::cout << std::format(
                    "{:>7.2f} {:>9.1f} {:>7.4f} {:>9.3f} {:>8.2f} {:>10.2f} {:>12.2f}\n", t,
                    setpoint, cart.x, pTerm, dTerm, asked, applied);
            }
            for (int s = 0; s < 100; ++s) {
                cart.step(applied, period / 100);
            }
        }
    }
    return 0;
}
