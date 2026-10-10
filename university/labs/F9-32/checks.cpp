// F9-32 checks: the worked example and the steady-state formulas, recomputed.
#include <cmath>
#include <iomanip>
#include <iostream>

int main()
{
    std::cout << std::fixed << std::setprecision(4);
    // Worked example: prior 10.0 m (sd 0.5), reading 10.6 m (sd 0.3).
    double x = 10.0, P = 0.5 * 0.5;
    const double z = 10.6, R = 0.3 * 0.3;
    double K = P / (P + R);
    x = x + K * (z - x);
    P = (1.0 - K) * P;
    std::cout << "update: K = " << K << ", x = " << x << " m, P = " << P << " m^2, sd = "
              << std::sqrt(P) << " m\n";
    // Same answer from the inverse-variance weighted mean.
    const double w1 = 1.0 / 0.25, w2 = 1.0 / 0.09;
    std::cout << "weighted mean: " << (w1 * 10.0 + w2 * 10.6) / (w1 + w2) << " m, variance "
              << 1.0 / (w1 + w2) << '\n';
    // Predict: move 2.0 m, motion sd 0.2 m.
    x += 2.0;
    P += 0.2 * 0.2;
    std::cout << "predict: x = " << x << " m, P = " << P << " m^2, sd = " << std::sqrt(P) << " m\n";
    // Second reading 12.3 m (sd 0.3).
    K = P / (P + R);
    x = x + K * (12.3 - x);
    P = (1.0 - K) * P;
    std::cout << "update 2: K = " << K << ", x = " << x << " m, sd = " << std::sqrt(P) << " m\n";

    // Steady state for the lab's tuned values: Ppred^2 - Q Ppred - Q R = 0.
    const double Q = 0.000344, Rl = 0.007044;
    const double Ppred = (Q + std::sqrt(Q * Q + 4.0 * Q * Rl)) / 2.0;
    const double Kss = Ppred / (Ppred + Rl);
    std::cout << std::setprecision(6) << "steady state: P_pred = " << Ppred << ", K = " << Kss
              << ", P = " << (1.0 - Kss) * Ppred << " (sd " << std::sqrt((1.0 - Kss) * Ppred)
              << " m), sqrt(S) = " << std::sqrt(Ppred + Rl) << " m\n";
    // Expected NIS if R is set 110 times too small (0.000064 instead of 0.007044).
    const double Rbad = 0.000064;
    const double Pb = (Q + std::sqrt(Q * Q + 4.0 * Q * Rbad)) / 2.0;
    const double Kb = Pb / (Pb + Rbad);
    std::cout << "with R = 0.000064: K = " << Kb << ", sqrt(P) = " << std::sqrt((1.0 - Kb) * Pb)
              << ", S = " << Pb + Rbad << ", true innovation variance about "
              << Pb + Rl << " (ignoring that the bad filter's P is not its real error),"
              << " ratio " << (Pb + Rl) / (Pb + Rbad) << '\n';
    std::cout << "R ratio 0.007044 / 0.000064 = " << Rl / Rbad << '\n';
    // NIS bounds for one degree of freedom: P(nu^2/S > 4) = erfc(2/sqrt 2).
    std::cout << "P(|nu| > 2 sqrt(S)) for a consistent filter = " << std::erfc(2.0 / std::sqrt(2.0))
              << '\n';
    // Lag-1 autocorrelation of white noise differenced once: -1/2.
    std::cout << "lag-1 autocorrelation of a differenced white sequence = " << -0.5 << '\n';
    return 0;
}
