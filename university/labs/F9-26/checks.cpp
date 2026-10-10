// F9-26 checks: worked example and answers, recomputed; also the c2 values behind the
// forensic log (printed with all digits) and the fixed function's results.
#include "ik.hpp"
#include <cstdio>

int main()
{
    const double L1 = 0.30, L2 = 0.25;
    const double x = 0.40, y = 0.20;
    const double c2 = (x * x + y * y - L1 * L1 - L2 * L2) / (2 * L1 * L2);
    std::printf("worked: d^2 = %.4f, c2 = %.6f, |s2| = %.6f\n", x * x + y * y, c2,
                std::sqrt(1 - c2 * c2));
    std::printf("worked: atan2(y, x) = %.4f deg, beta(down) = %.4f deg\n", deg(std::atan2(y, x)),
                deg(std::atan2(L2 * std::sqrt(1 - c2 * c2), L1 + L2 * c2)));
    for (int a : {-40, -30, -20, -10, 0, 10, 20, 30, 40}) {
        const double p = a * std::numbers::pi / 180.0, xx = 0.55 * std::cos(p),
                     yy = 0.55 * std::sin(p);
        const double c = (xx * xx + yy * yy - L1 * L1 - L2 * L2) / (2 * L1 * L2);
        const auto s = ik2r(L1, L2, xx, yy, false);
        std::printf("forensic: angle %4d  c2 = %.17g  fixed ik2r: t1 %.3f t2 %.3f\n", a, c,
                    deg(s->t1), deg(s->t2));
    }
    // answer: target (0, 0.30) with L1 = 0.30, L2 = 0.25
    for (bool up : {true, false}) {
        const auto s = ik2r(L1, L2, 0.0, 0.30, up);
        std::printf("answer (0, 0.30) %s: t1 %.3f t2 %.3f\n", up ? "up  " : "down", deg(s->t1),
                    deg(s->t2));
    }
    return 0;
}
