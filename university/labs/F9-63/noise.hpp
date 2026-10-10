// noise.hpp - reproducible pseudo-random numbers for the F9-63 labs (fixed seed, Box-Muller on
// the raw std::mt19937 output, so the same toolchain prints the same numbers every run).
#pragma once
#include <cmath>
#include <numbers>
#include <random>

class Noise
{
public:
    explicit Noise(unsigned seed) : gen_(seed) {}
    double uniform() { return (static_cast<double>(gen_()) + 0.5) / 4294967296.0; }  // (0, 1)
    double gauss()
    {
        const double u1 = uniform();
        const double u2 = uniform();
        return std::sqrt(-2.0 * std::log(u1)) * std::cos(2.0 * std::numbers::pi * u2);
    }

private:
    std::mt19937 gen_;
};

// An encoder reports whole counts: round to the nearest multiple of the step.
inline double quantise(double v, double step) { return step * std::round(v / step); }
