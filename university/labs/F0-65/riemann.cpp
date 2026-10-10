// F0-65 Listing 1: distance travelled as the integral of speed. The speed of a cart
// that starts from rest is v(t) = 2 (1 - e^(-t/2)) m/s; we add up thin strips four ways
// and compare with the exact integral from 0 to 4 s, 4 + 4 e^(-2) metres.
#include <cmath>
#include <format>
#include <iostream>

double speed(double t)
{
    return 2.0 * (1.0 - std::exp(-t / 2.0));
}

int main()
{
    const double a = 0.0;
    const double b = 4.0;
    const double exact = 4.0 + 4.0 * std::exp(-2.0);
    std::cout << std::format("exact distance = {:.9f} m\n", exact);
    std::cout << "    N        left       right    midpoint   trapezoid   | errors: left      trapezoid\n";
    for (int n = 4; n <= 1024; n *= 4) {
        const double h = (b - a) / n;
        double left = 0.0;
        double right = 0.0;
        double mid = 0.0;
        for (int k = 0; k < n; ++k) {
            const double t = a + k * h;
            left += speed(t) * h;
            right += speed(t + h) * h;
            mid += speed(t + h / 2.0) * h;
        }
        const double trap = (left + right) / 2.0;
        std::cout << std::format("{:>5} {:>11.7f} {:>11.7f} {:>11.7f} {:>11.7f}   | {:>12.3e} {:>12.3e}\n", n, left,
                                 right, mid, trap, left - exact, trap - exact);
    }
    return 0;
}
