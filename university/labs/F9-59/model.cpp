// model.cpp - the cart-pole in state-space form: A, B, open-loop poles, and the
// discrete-time model a 100 Hz controller sees.
#include <cmath>
#include <cstdio>
#include "cartpole.hpp"
#include "lin.hpp"

int main()
{
    const CartPole p;  // M = 1 kg, m = 0.2 kg, l = 0.5 m, b = 0.1 N s/m
    const Mat A = cartA(p);
    const Mat B = cartB(p);
    printMat("A", A);
    printMat("B", B);

    const auto poly = charpoly(A);
    std::printf("det(sI - A) = s^4 %+.4f s^3 %+.4f s^2 %+.4f s %+.4f\n",
                poly[1], poly[2], poly[3], poly[4]);
    const auto s = eig(A);
    printEig("open-loop poles s", s);

    const double T = 0.01;  // sample period of the digital controller, s
    const auto [Ad, Bd] = c2d(A, B, T);
    printMat("Ad (T = 0.01 s)", Ad);
    printMat("Bd (T = 0.01 s)", Bd);
    const auto z = eig(Ad);
    printEig("discrete poles z", z);
    std::printf("check z = e^(sT):");
    for (const auto& si : s) std::printf("  %.6f", std::exp(si.real() * T));
    std::printf("\n");
    std::printf("unstable pole doubles the lean every %.3f s\n", std::log(2.0) / s[0].real());
    return 0;
}
