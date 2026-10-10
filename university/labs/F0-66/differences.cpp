// F0-66 Listing 1: forward, backward and central differences for the derivative of
// sin(t) at t = 1 (exact answer cos 1). The error shrinks like h for the one-sided
// differences and like h^2 for the central difference.
#include <cmath>
#include <format>
#include <iostream>

int main()
{
    const double t = 1.0;
    const double exact = std::cos(t);
    std::cout << std::format("exact derivative cos(1) = {:.12f}\n", exact);
    std::cout << "      h    forward error   backward error    central error\n";
    for (double h = 0.1; h > 1e-6; h /= 10.0) {
        const double fwd = (std::sin(t + h) - std::sin(t)) / h;
        const double bwd = (std::sin(t) - std::sin(t - h)) / h;
        const double ctr = (std::sin(t + h) - std::sin(t - h)) / (2.0 * h);
        std::cout << std::format("{:>7.0e} {:>16.3e} {:>16.3e} {:>16.3e}\n", h, fwd - exact, bwd - exact, ctr - exact);
    }
    return 0;
}
