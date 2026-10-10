// F1-72: deterministic Gaussian noise for the university's noise models
// (a small linear congruential generator plus the Box-Muller transform), so every
// run prints the same numbers.
#pragma once
#include <cmath>
#include <cstdint>
#include <numbers>

class Noise
{
public:
    explicit Noise(std::uint32_t seed) : state_(seed) {}
    double uniform()
    {
        state_ = state_ * 1664525u + 1013904223u;
        return (static_cast<double>(state_ >> 8) + 0.5) / 16777216.0;
    }
    double gaussian(double sigma)
    {
        const double u1 = uniform();
        const double u2 = uniform();
        return sigma * std::sqrt(-2.0 * std::log(u1)) * std::cos(2.0 * std::numbers::pi * u2);
    }

private:
    std::uint32_t state_;
};
