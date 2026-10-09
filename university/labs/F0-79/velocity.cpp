// F0-79 worked example check: the Tustin PI controller in positional form and in the incremental
// ("velocity") form u[k] = u[k-1] + q0 e[k] + q1 e[k-1] give the same outputs.
#include <cmath>
#include <cstdio>

int main()
{
    const double kp = 2.0, ki = 8.0, T = 0.05;
    const double q0 = kp + ki * T / 2.0, q1 = -kp + ki * T / 2.0;
    std::printf("q0 = Kp + Ki T/2 = %.4f, q1 = -Kp + Ki T/2 = %.4f\n", q0, q1);
    const double e[] = {1.0, 0.8, 0.5, 0.2, -0.1, -0.05, 0.0};
    double integ = 0.0, ePrev = 0.0, uPrev = 0.0, worst = 0.0;
    std::printf("%-3s %-7s %-12s %-12s\n", "k", "e[k]", "positional", "incremental");
    for (int k = 0; k < 7; ++k) {
        integ += T * 0.5 * (e[k] + ePrev);
        const double uPos = kp * e[k] + ki * integ;
        const double uInc = uPrev + q0 * e[k] + q1 * ePrev;
        std::printf("%-3d %-7.2f %-12.6f %-12.6f\n", k, e[k], uPos, uInc);
        worst = std::fmax(worst, std::fabs(uPos - uInc));
        ePrev = e[k];
        uPrev = uInc;
    }
    std::printf("largest difference: %.3g\n", worst);
    return worst < 1e-12 ? 0 : 1;
}
