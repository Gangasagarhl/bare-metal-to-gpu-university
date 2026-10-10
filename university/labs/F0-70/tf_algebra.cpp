// F0-70 Listing 1: transfer functions as pairs of polynomials, and the three block-diagram
// rules: series G1 G2, parallel G1 + G2, negative feedback G / (1 + G H).
// Polynomials are stored lowest power first: {a0, a1, a2} means a0 + a1 s + a2 s^2.
#include <algorithm>
#include <cstddef>
#include <format>
#include <iostream>
#include <string>
#include <vector>

using Poly = std::vector<double>;

struct Tf
{
    Poly num;
    Poly den;
};

Poly mul(const Poly& a, const Poly& b)
{
    Poly r(a.size() + b.size() - 1, 0.0);
    for (std::size_t i = 0; i < a.size(); ++i) {
        for (std::size_t j = 0; j < b.size(); ++j) {
            r[i + j] += a[i] * b[j];
        }
    }
    return r;
}

Poly add(const Poly& a, const Poly& b)
{
    Poly r(std::max(a.size(), b.size()), 0.0);
    for (std::size_t i = 0; i < a.size(); ++i) {
        r[i] += a[i];
    }
    for (std::size_t i = 0; i < b.size(); ++i) {
        r[i] += b[i];
    }
    return r;
}

Tf series(const Tf& g1, const Tf& g2)
{
    return {mul(g1.num, g2.num), mul(g1.den, g2.den)};
}

Tf parallel(const Tf& g1, const Tf& g2)
{
    return {add(mul(g1.num, g2.den), mul(g2.num, g1.den)), mul(g1.den, g2.den)};
}

Tf feedback(const Tf& g, const Tf& h)   // negative feedback: G / (1 + G H)
{
    return {mul(g.num, h.den), add(mul(g.den, h.den), mul(g.num, h.num))};
}

std::string show(const Poly& p)
{
    std::string s;
    for (std::size_t i = p.size(); i-- > 0;) {
        if (!s.empty()) {
            s += " + ";
        }
        s += std::format("{:.6g}", p[i]);
        if (i == 1) {
            s += " s";
        } else if (i > 1) {
            s += std::format(" s^{}", i);
        }
    }
    return s;
}

void print(const std::string& name, const Tf& g)
{
    std::cout << std::format("{:<26} ({}) / ({})\n", name, show(g.num), show(g.den));
}

int main()
{
    const Tf motor{{98.039}, {1.0, 0.1961}};     // speed per volt: 98.039 / (0.1961 s + 1)
    const Tf gainP{{0.05}, {1.0}};                 // proportional controller, V per (rad/s)
    const Tf sensor{{1.0}, {1.0}};                 // ideal speed sensor
    const Tf integrator{{1.0}, {0.0, 1.0}};        // 1/s: speed -> angle
    print("motor G(s)", motor);
    print("controller C(s)", gainP);
    const Tf open = series(gainP, motor);
    print("open loop C G", open);
    const Tf closed = feedback(open, sensor);
    print("closed loop CG/(1+CG)", closed);
    std::cout << std::format("closed-loop DC gain T(0) = {:.4f}\n", closed.num[0] / closed.den[0]);
    std::cout << std::format("closed-loop time constant = {:.5f} s\n", closed.den[1] / closed.den[0]);
    print("angle = (1/s) G", series(integrator, motor));
    print("parallel 1/(s+1) + 2/(s+3)", parallel(Tf{{1.0}, {1.0, 1.0}}, Tf{{2.0}, {3.0, 1.0}}));
    return 0;
}
