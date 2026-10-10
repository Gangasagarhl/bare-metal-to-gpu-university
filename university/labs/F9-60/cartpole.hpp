// cartpole.hpp - the RB402 cart-pole: a cart on a straight track with a pole hinged on top.
// Exercise values, not measurements of any real rig (see F9-59).
#pragma once
#include <array>
#include <cmath>
#include "lin.hpp"

struct CartPole
{
    double M = 1.0;   // cart mass, kg
    double m = 0.2;   // mass at the top of the pole, kg (the rod is treated as massless)
    double l = 0.5;   // hinge-to-mass length, m
    double b = 0.1;   // cart friction, N s/m
    double g = 9.81;  // gravity, m/s^2
};

// State s = (x, v, th, w): cart position, cart velocity, pole angle from upright (rad,
// positive = leaning towards +x), pole angular velocity. Input F = horizontal force on the cart.
using State = std::array<double, 4>;

// Full nonlinear equations of motion (Lagrange; derived in F9-59 Layer 2).
inline State deriv(const CartPole& p, const State& s, double F)
{
    const double v = s[1];
    const double th = s[2];
    const double w = s[3];
    const double sn = std::sin(th);
    const double cs = std::cos(th);
    // (M+m) a + m l cos(th) al = F - b v + m l sin(th) w^2
    //      cos(th) a +     l al = g sin(th)
    const double rhs1 = F - p.b * v + p.m * p.l * sn * w * w;
    const double rhs2 = p.g * sn;
    const double det = (p.M + p.m) * p.l - p.m * p.l * cs * cs;
    const double acc = (rhs1 * p.l - p.m * p.l * cs * rhs2) / det;
    const double alpha = ((p.M + p.m) * rhs2 - cs * rhs1) / det;
    return {v, acc, w, alpha};
}

// One classical Runge-Kutta (RK4) step of length h with the force held constant.
inline State rk4(const CartPole& p, const State& s, double F, double h)
{
    auto add = [](const State& a, const State& d, double k) {
        return State{a[0] + k * d[0], a[1] + k * d[1], a[2] + k * d[2], a[3] + k * d[3]};
    };
    const State k1 = deriv(p, s, F);
    const State k2 = deriv(p, add(s, k1, h / 2), F);
    const State k3 = deriv(p, add(s, k2, h / 2), F);
    const State k4 = deriv(p, add(s, k3, h), F);
    State out{};
    for (int i = 0; i < 4; ++i) out[i] = s[i] + h / 6.0 * (k1[i] + 2 * k2[i] + 2 * k3[i] + k4[i]);
    return out;
}

// Linearisation about the upright equilibrium s = 0, F = 0:  s' = A s + B F.
inline Mat cartA(const CartPole& p)
{
    return Mat(4, 4, {0, 1, 0, 0,
                      0, -p.b / p.M, -p.m * p.g / p.M, 0,
                      0, 0, 0, 1,
                      0, p.b / (p.M * p.l), (p.M + p.m) * p.g / (p.M * p.l), 0});
}

inline Mat cartB(const CartPole& p) { return Mat(4, 1, {0, 1.0 / p.M, 0, -1.0 / (p.M * p.l)}); }
