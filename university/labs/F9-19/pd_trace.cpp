// F9-19 Listing 2: cart position for Kd = 0 and Kd = 4.8 (Kp = 8), every 0.1 s.
// Used for Figure 2. Same cart and controller as Listing 1.
#include <algorithm>
#include <array>
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
    const std::array<double, 2> gainsD = {0.0, 4.8};
    std::array<Cart, 2> carts;
    std::array<double, 2> previousX = {0.0, 0.0};
    std::cout << "  t (s)   x with Kd=0 (m)   x with Kd=4.8 (m)\n";
    for (int k = 0; k <= 800; ++k) {
        if (k % 10 == 0) {
            std::cout << std::format("{:>7.1f} {:>17.3f} {:>19.3f}\n", k * 0.01, carts[0].x,
                                     carts[1].x);
        }
        for (int c = 0; c < 2; ++c) {
            const double x = carts[c].x;
            const double dTerm = -gainsD[c] * (x - previousX[c]) / 0.01;
            previousX[c] = x;
            const double force = std::clamp(kp * (1.0 - x) + dTerm, -10.0, 10.0);
            for (int s = 0; s < 100; ++s) {
                carts[c].step(force, 0.0001);
            }
        }
    }
    return 0;
}
