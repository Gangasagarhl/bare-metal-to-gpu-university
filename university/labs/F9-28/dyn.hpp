// F9-28 Listing 1: dynamics of a planar two-link arm moving in a vertical plane.
// Model: point masses m1 at the elbow and m2 at the tool, massless links, no friction.
// Equation of motion: M(q) qdd + c(q, qd) + g(q) = tau.
#pragma once
#include <array>
#include <cmath>

using V2 = std::array<double, 2>;
using M2 = std::array<V2, 2>;

struct Arm2 {
    double L1 = 0.30, L2 = 0.25; // link lengths (m)
    double m1 = 1.0, m2 = 0.5;   // point masses (kg) of the simulated arm
    double g = 9.81;             // gravity used by this model (m/s^2), acting along -y

    M2 mass(const V2& q) const
    {
        const double c2 = std::cos(q[1]);
        const double m11 = (m1 + m2) * L1 * L1 + m2 * L2 * L2 + 2 * m2 * L1 * L2 * c2;
        const double m12 = m2 * L2 * L2 + m2 * L1 * L2 * c2;
        return {{{m11, m12}, {m12, m2 * L2 * L2}}};
    }
    V2 coriolis(const V2& q, const V2& qd) const // Coriolis and centrifugal torques
    {
        const double h = m2 * L1 * L2 * std::sin(q[1]);
        return {-h * (2 * qd[0] * qd[1] + qd[1] * qd[1]), h * qd[0] * qd[0]};
    }
    V2 gravity(const V2& q) const // torques that gravity demands at each joint
    {
        const double c1 = std::cos(q[0]), c12 = std::cos(q[0] + q[1]);
        return {(m1 + m2) * g * L1 * c1 + m2 * g * L2 * c12, m2 * g * L2 * c12};
    }
    double energy(const V2& q, const V2& qd) const // kinetic + potential (J)
    {
        const M2 M = mass(q);
        const double ke =
            0.5 * (M[0][0] * qd[0] * qd[0] + 2 * M[0][1] * qd[0] * qd[1] + M[1][1] * qd[1] * qd[1]);
        const double pe = (m1 + m2) * g * L1 * std::sin(q[0]) + m2 * g * L2 * std::sin(q[0] + q[1]);
        return ke + pe;
    }
    V2 inverseDyn(const V2& q, const V2& qd, const V2& qdd) const // tau from motion
    {
        const M2 M = mass(q);
        const V2 c = coriolis(q, qd), gr = gravity(q);
        return {M[0][0] * qdd[0] + M[0][1] * qdd[1] + c[0] + gr[0],
                M[1][0] * qdd[0] + M[1][1] * qdd[1] + c[1] + gr[1]};
    }
    V2 forwardDyn(const V2& q, const V2& qd, const V2& tau) const // motion from tau
    {
        const M2 M = mass(q);
        const V2 c = coriolis(q, qd), gr = gravity(q);
        const V2 b{tau[0] - c[0] - gr[0], tau[1] - c[1] - gr[1]};
        const double d = M[0][0] * M[1][1] - M[0][1] * M[1][0]; // M is always invertible
        return {(M[1][1] * b[0] - M[0][1] * b[1]) / d, (-M[1][0] * b[0] + M[0][0] * b[1]) / d};
    }
};

struct State {
    V2 q, qd;
};

// one fourth-order Runge-Kutta step with constant torque tau (MA302)
inline State rk4(const Arm2& a, const State& s, const V2& tau, double h)
{
    auto f = [&](const State& x) { return State{x.qd, a.forwardDyn(x.q, x.qd, tau)}; };
    auto add = [](const State& x, const State& k, double w) {
        return State{{x.q[0] + w * k.q[0], x.q[1] + w * k.q[1]},
                     {x.qd[0] + w * k.qd[0], x.qd[1] + w * k.qd[1]}};
    };
    const State k1 = f(s), k2 = f(add(s, k1, h / 2)), k3 = f(add(s, k2, h / 2)),
                k4 = f(add(s, k3, h));
    State n = s;
    for (int i = 0; i < 2; ++i) {
        n.q[i] += h / 6 * (k1.q[i] + 2 * k2.q[i] + 2 * k3.q[i] + k4.q[i]);
        n.qd[i] += h / 6 * (k1.qd[i] + 2 * k2.qd[i] + 2 * k3.qd[i] + k4.qd[i]);
    }
    return n;
}
