// checks.cpp - recomputes the numbers quoted in the text of F9-63.
#include <cmath>
#include <complex>
#include <cstdio>
#include <vector>
#include "cartpole.hpp"
#include "lin.hpp"
#include "lqr.hpp"
#include "place.hpp"

using cd = std::complex<double>;

int main()
{
    // 1. Worked example: the F0-67 cart at T = 0.1 s, position sensor, observer poles 0.5, 0.6.
    const auto [Ac, Bc] = c2d(Mat(2, 2, {0, 1, 0, -0.4}), Mat(2, 1, {0, 0.5}), 0.1);
    const Mat Cc(1, 2, {1, 0});
    const Mat Lc = tr(acker(tr(Ac), tr(Cc), {0.5, 0.6}));
    const double l1 = 1.0 + Ac(1, 1) - 1.1;
    const double l2 = (0.3 - (1.0 - l1) * Ac(1, 1)) / Ac(0, 1);
    std::printf("cart observer: L = (%.6f, %.6f), by hand (%.6f, %.6f)\n",
                Lc(0, 0), Lc(1, 0), l1, l2);
    printEig("cart: eig(Ad - L C)", eig(Ac - Lc * Cc));

    // 2. Separation: the closed loop (x, e) has the controller's and the observer's poles.
    const CartPole p;
    const double T = 0.01;
    const auto [Ad, Bd] = c2d(cartA(p), cartB(p), T);
    const Mat C(1, 4, {1, 0, 0, 0});
    const Mat K = dlqr(Ad, Bd, diag({25.0, 1.0, 100.0, 1.0}), diag({0.01})).K;
    std::vector<cd> zo;
    for (double s : {-8.0, -9.0, -10.0, -11.0}) zo.push_back(std::exp(s * T));
    const Mat L = tr(acker(tr(Ad), tr(C), zo));
    // x[k+1] = Ad x - Bd K (x - e);  e[k+1] = (Ad - L C) e
    Mat big(8, 8);
    const Mat AK = Ad - Bd * K;
    const Mat BK = Bd * K;
    const Mat AL = Ad - L * C;
    for (std::size_t i = 0; i < 4; ++i)
        for (std::size_t j = 0; j < 4; ++j) {
            big(i, j) = AK(i, j);
            big(i, 4 + j) = BK(i, j);
            big(4 + i, 4 + j) = AL(i, j);
        }
    printEig("eig(Ad - Bd K)", eig(AK));
    printEig("eig(Ad - L C)", eig(AL));
    // det(zI - big) should equal det(zI - AK) * det(zI - AL): compare the coefficients.
    const auto pk = charpoly(AK);
    const auto pl = charpoly(AL);
    std::vector<double> prod(pk.size() + pl.size() - 1, 0.0);
    for (std::size_t i = 0; i < pk.size(); ++i)
        for (std::size_t j = 0; j < pl.size(); ++j) prod[i + j] += pk[i] * pl[j];
    const auto pb = charpoly(big);
    double diff = 0.0;
    for (std::size_t i = 0; i < pb.size(); ++i) diff = std::fmax(diff, std::fabs(pb[i] - prod[i]));
    std::printf("combined 8x8: largest coefficient difference from the product = %.1e\n", diff);
    std::printf("observer time constants: ");
    for (const auto& z : zo) std::printf(" %.4f s", -T / std::log(z.real()));
    std::printf("\n");
    // 3. |z| to time constant for the forensic boot log
    for (double m : {0.960, 0.815, 0.241, 0.116})
        std::printf("|z| = %.3f -> time constant -T/ln|z| = %.4f s\n", m, -T / std::log(m));
    return 0;
}
