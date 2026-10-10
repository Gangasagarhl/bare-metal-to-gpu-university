// F0-76 shared code: the pendulum theta'' = -(g/L) sin(theta) and three ways to step it forward.
#pragma once
#include <cmath>

struct State
{
    double theta; // angle from straight down (rad)
    double omega; // angular velocity (rad/s)
};

constexpr double kG = 9.81; // m/s^2 (the value this course uses; not a measured constant)
constexpr double kL = 1.0;  // m

// The right-hand side f(state): d(theta)/dt = omega, d(omega)/dt = -(g/L) sin(theta).
inline State deriv(const State& s)
{
    return {s.omega, -(kG / kL) * std::sin(s.theta)};
}

// Energy per unit mass, divided by L^2: (1/2) omega^2 + (g/L)(1 - cos theta).
// It is constant for the true motion.
inline double energy(const State& s)
{
    return 0.5 * s.omega * s.omega + (kG / kL) * (1.0 - std::cos(s.theta));
}

// Explicit (forward) Euler: follow the slope at the start of the step for the whole step.
inline State eulerStep(const State& s, double h)
{
    const State d = deriv(s);
    return {s.theta + h * d.theta, s.omega + h * d.omega};
}

// Semi-implicit (symplectic) Euler: update omega first, then use the NEW omega for theta.
inline State semiImplicitStep(const State& s, double h)
{
    const double omega = s.omega - h * (kG / kL) * std::sin(s.theta);
    return {s.theta + h * omega, omega};
}

// Classical fourth-order Runge-Kutta: four slope samples per step, weighted 1, 2, 2, 1.
inline State rk4Step(const State& s, double h)
{
    const State k1 = deriv(s);
    const State k2 = deriv({s.theta + 0.5 * h * k1.theta, s.omega + 0.5 * h * k1.omega});
    const State k3 = deriv({s.theta + 0.5 * h * k2.theta, s.omega + 0.5 * h * k2.omega});
    const State k4 = deriv({s.theta + h * k3.theta, s.omega + h * k3.omega});
    return {s.theta + h / 6.0 * (k1.theta + 2.0 * k2.theta + 2.0 * k3.theta + k4.theta),
            s.omega + h / 6.0 * (k1.omega + 2.0 * k2.omega + 2.0 * k3.omega + k4.omega)};
}
