// place.cpp - place the closed-loop poles of the cart-pole, check them, and balance the
// nonlinear cart-pole from a 0.1 rad lean with a controller running at 100 Hz.
#include <cmath>
#include <complex>
#include <cstdio>
#include <vector>
#include "cartpole.hpp"
#include "lin.hpp"
#include "place.hpp"

using cd = std::complex<double>;

static void balance(const CartPole& p, const Mat& K, double T, const char* name)
{
    State s{0.0, 0.0, 0.1, 0.0};
    double peakF = 0.0;
    std::printf("%s, T = %.3f s\n   t(s)     x(m)   th(rad)    F(N)\n", name, T);
    const int steps = static_cast<int>(std::lround(4.0 / T));
    for (int k = 0; k <= steps; ++k) {
        double F = 0.0;
        for (std::size_t i = 0; i < 4; ++i) F -= K(0, i) * s[i];  // u = -K x
        peakF = std::fmax(peakF, std::fabs(F));
        if (k % static_cast<int>(std::lround(0.5 / T)) == 0)
            std::printf("  %5.2f  %7.4f  %8.4f  %6.2f\n", k * T, s[0], s[2], F);
        for (int i = 0; i < 10; ++i) s = rk4(p, s, F, T / 10);  // force held for one period
    }
    std::printf("  peak |F| = %.2f N\n", peakF);
}

int main()
{
    const CartPole p;
    const Mat A = cartA(p);
    const Mat B = cartB(p);
    const std::vector<cd> want{cd(-2, 1), cd(-2, -1), -5.0, -6.0};
    const Mat K = acker(A, B, want);
    printMat("K (continuous design)", K);
    printEig("eig(A - B K)", eig(A - B * K));

    const double T = 0.01;
    const auto [Ad, Bd] = c2d(A, B, T);
    std::vector<cd> wantZ;
    for (const auto& s : want) wantZ.push_back(std::exp(s * T));  // z = e^(sT)
    const Mat Kd = acker(Ad, Bd, wantZ);
    printMat("Kd (discrete design, z = e^(sT))", Kd);
    printEig("eig(Ad - Bd Kd)", eig(Ad - Bd * Kd));
    printEig("eig(Ad - Bd K)  (continuous K run at 100 Hz)", eig(Ad - Bd * K));

    balance(p, Kd, T, "nonlinear cart-pole, discrete design");
    return 0;
}
