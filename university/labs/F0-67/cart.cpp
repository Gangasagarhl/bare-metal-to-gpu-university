// F0-67 Listing 1 (course lab): simulate a cart with viscous friction and compare it
// with the analytic solution of m v' = F - c v, v(0) = 0, x(0) = 0:
//   v(t) = (F/c) (1 - e^(-t/tau)),  x(t) = (F/c) (t - tau (1 - e^(-t/tau))),  tau = m/c.
#include <cmath>
#include <format>
#include <iostream>

struct Cart
{
    double mass = 2.0;       // kg
    double friction = 0.8;   // N per (m/s), viscous friction coefficient c
    double x = 0.0;          // m
    double v = 0.0;          // m/s
};

// One small time step: the rates of change from the differential equation, times dt.
void step(Cart& cart, double force, double dt)
{
    const double a = (force - cart.friction * cart.v) / cart.mass;
    cart.x += cart.v * dt;
    cart.v += a * dt;
}

double maxSpeedError(double dt, double force, double tEnd)
{
    Cart cart;
    const double tau = cart.mass / cart.friction;
    const double vInf = force / cart.friction;
    double worst = 0.0;
    const int n = static_cast<int>(std::lround(tEnd / dt));
    for (int k = 1; k <= n; ++k) {
        step(cart, force, dt);
        const double t = k * dt;
        const double exact = vInf * (1.0 - std::exp(-t / tau));
        worst = std::max(worst, std::abs(cart.v - exact));
    }
    return worst;
}

int main()
{
    const double force = 1.6;   // N, a steady push
    const double dt = 0.001;    // s
    Cart cart;
    const double tau = cart.mass / cart.friction;
    const double vInf = force / cart.friction;
    std::cout << std::format("m = {} kg, c = {} N s/m, F = {} N: tau = m/c = {} s, terminal speed F/c = {} m/s\n",
                             cart.mass, cart.friction, force, tau, vInf);
    std::cout << "  t (s)   v sim    v exact    x sim    x exact\n";
    const int n = static_cast<int>(std::lround(10.0 / dt));
    for (int k = 0; k <= n; ++k) {
        const double t = k * dt;
        if (k % 1000 == 0) {
            const double vExact = vInf * (1.0 - std::exp(-t / tau));
            const double xExact = vInf * (t - tau * (1.0 - std::exp(-t / tau)));
            std::cout << std::format("{:>7.1f} {:>8.4f} {:>9.4f} {:>8.4f} {:>9.4f}\n", t, cart.v, vExact, cart.x,
                                     xExact);
        }
        step(cart, force, dt);
    }
    std::cout << "time step dt (s)   largest speed error over 0..10 s (m/s)\n";
    double previous = 0.0;
    for (double h : {0.1, 0.01, 0.001}) {
        const double err = maxSpeedError(h, force, 10.0);
        std::cout << std::format("{:>16} {:>14.3e}", h, err);
        if (previous > 0.0) {
            std::cout << std::format("   ({:.1f} times smaller)", previous / err);
        }
        std::cout << "\n";
        previous = err;
    }
    const double errFine = maxSpeedError(0.001, force, 10.0);
    const bool pass = errFine < 1e-3;
    std::cout << (pass ? "PASS" : "FAIL") << ": simulation within 1e-3 m/s of the analytic solution at dt = 0.001 s\n";
    return pass ? 0 : 1;
}
