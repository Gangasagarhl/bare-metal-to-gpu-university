// units.cpp - F12-19 Listing 4: the same number in two units, and a type that refuses to mix
// them. 1 lbf = 0.45359237 kg * 9.80665 m/s^2 (the definitions of the international pound
// and of standard gravity), so 1 lbf*s = 4.4482216152605 N*s.
#include <cstdio>

struct NewtonSeconds
{
    double value;
};

struct PoundForceSeconds
{
    double value;
};

constexpr double kNewtonPerPoundForce = 0.45359237 * 9.80665;

constexpr NewtonSeconds to_newton_seconds(PoundForceSeconds p)
{
    return NewtonSeconds{p.value * kNewtonPerPoundForce};
}

// The consumer of the number: the type says which unit it expects.
double velocity_change(NewtonSeconds impulse, double mass_kg)
{
    return impulse.value / mass_kg;
}

int main()
{
    std::printf("1 lbf = %.13f N\n", kNewtonPerPoundForce);
    const double reported = 100.0;  // producer reports 100, meaning pound-force seconds
    const double mass = 50.0;       // kg, model value
    // Untyped: the consumer reads the bare number as newton seconds.
    std::printf("untyped: delta-v = %.3f m/s (100 read as N*s)\n", reported / mass);
    // Typed: the producer's unit is part of the type; the conversion is explicit.
    const PoundForceSeconds p{reported};
    std::printf("typed:   delta-v = %.3f m/s (100 lbf*s converted)\n",
                velocity_change(to_newton_seconds(p), mass));
    std::printf("ratio typed / untyped = %.4f\n",
                velocity_change(to_newton_seconds(p), mass) / (reported / mass));
    return 0;
}
