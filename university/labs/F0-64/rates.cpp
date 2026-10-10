// F0-64 Listing 1: the average rate of change of a cart's position over
// shrinking time intervals approaches the derivative (the instantaneous rate).
#include <cmath>
#include <format>
#include <iostream>

// Position of the toy cart in metres after t seconds (the chapter's example).
double position(double t)
{
    return 0.3 * t * t + 0.5 * t;
}

int main()
{
    const double t0 = 1.0;
    const double exact = 0.6 * t0 + 0.5;   // derivative x'(t) = 0.6 t + 0.5
    std::cout << std::format("x(t) = 0.3 t^2 + 0.5 t, at t0 = {} s; exact x'(t0) = {} m/s\n", t0, exact);
    std::cout << "        h (s)   average rate (m/s)        error (m/s)\n";
    for (int k = 0; k <= 12; ++k) {
        const double h = std::pow(10.0, -k);
        const double rate = (position(t0 + h) - position(t0)) / h;
        std::cout << std::format("{:>13.0e}   {:>18.12f}   {:>16.3e}\n", h, rate, rate - exact);
    }
    return 0;
}
