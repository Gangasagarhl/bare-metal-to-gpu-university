// checks.cpp - recomputes the numbers quoted in the text of F9-59.
#include <cmath>
#include <cstdio>
#include "cartpole.hpp"
#include "lin.hpp"

static double energy(const CartPole& p, const State& s)
{
    const double c = std::cos(s[2]);
    return 0.5 * (p.M + p.m) * s[1] * s[1] + p.m * p.l * c * s[1] * s[3]
         + 0.5 * p.m * p.l * p.l * s[3] * s[3] + p.m * p.g * p.l * c;
}

int main()
{
    // 1. The equations of motion conserve energy when friction and force are zero.
    CartPole p0;
    p0.b = 0.0;
    State s{0.0, 0.0, 0.3, 0.0};
    const double e0 = energy(p0, s);
    double worst = 0.0;
    for (int k = 0; k < 20000; ++k) {  // 2 s in 0.1 ms steps; the pole swings through the bottom
        s = rk4(p0, s, 0.0, 1e-4);
        worst = std::fmax(worst, std::fabs(energy(p0, s) - e0) / std::fabs(e0));
    }
    std::printf("energy check (b = 0): largest relative change over 2 s = %.1e\n", worst);

    // 2. Frictionless upright pendulum: theta/F = -1/(M l) / (s^2 - (M+m) g/(M l)).
    const double a43 = (p0.M + p0.m) * p0.g / (p0.M * p0.l);
    std::printf("frictionless: (M+m)g/(Ml) = %.4f, poles +/- %.4f 1/s\n", a43, std::sqrt(a43));
    printEig("frictionless eig(A)", eig(cartA(p0)));

    // 3. Worked example: the F0-67 cart (m = 2 kg, c = 0.8 N s/m), x and v, sampled at 0.1 s.
    const Mat A(2, 2, {0, 1, 0, -0.4});
    const Mat B(2, 1, {0, 0.5});
    const double T = 0.1;
    const auto [Ad, Bd] = c2d(A, B, T);
    const double e = std::exp(-0.4 * T);
    std::printf("cart c2d:  Ad12 = %.6f (hand %.6f)  Ad22 = %.6f (hand %.6f)\n",
                Ad(0, 1), (1 - e) / 0.4, Ad(1, 1), e);
    std::printf("           Bd1  = %.6f (hand %.6f)  Bd2  = %.6f (hand %.6f)\n",
                Bd(0, 0), 0.5 * (T / 0.4 - (1 - e) / 0.16), Bd(1, 0), 0.5 * (1 - e) / 0.4);
    // G(s) = C (sI - A)^-1 B at s = 1 and s = 2, against 0.5 / (s (s + 0.4)).
    for (double sv : {1.0, 2.0}) {
        const Mat C(1, 2, {1, 0});
        const Mat G = C * inv(sv * eye(2) - A) * B;
        std::printf("G(%.0f) = %.6f   0.5/(s(s+0.4)) = %.6f\n",
                    sv, G(0, 0), 0.5 / (sv * (sv + 0.4)));
    }

    // 4. Doubling time of the unstable mode and its discrete pole.
    const auto z = eig(cartA(CartPole{}));
    std::printf("doubling time ln2/%.4f = %.4f s, e^(sT) at T = 0.01: %.6f\n",
                z[0].real(), std::log(2.0) / z[0].real(), std::exp(z[0].real() * 0.01));
    return 0;
}
