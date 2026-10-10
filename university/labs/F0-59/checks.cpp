// F0-59 checks: recompute every number used in the chapter text.
#include <cmath>
#include <iostream>

// Standard Gaussian CDF: Phi(z) = P(Z <= z) = (1 + erf(z / sqrt 2)) / 2.
double phi(double z)
{
    return 0.5 * (1.0 + std::erf(z / std::sqrt(2.0)));
}

int main()
{
    const double pi = std::acos(-1.0);
    std::cout << "peak of N(20, 0.5^2) density: 1 / (0.5 sqrt(2 pi)) = "
              << 1.0 / (0.5 * std::sqrt(2.0 * pi)) << " per degree C\n";
    std::cout << "peak of N(0, 1) density: " << 1.0 / std::sqrt(2.0 * pi) << "\n";
    for (double z : {1.0, 1.5, 1.96, 2.0, 3.0}) {
        std::cout << "z = " << z << ": P(Z <= z) = " << phi(z) << ", P(|Z| < z) = "
                  << phi(z) - phi(-z) << ", P(Z > z) = " << 1.0 - phi(z) << "\n";
    }
    // Kitchen thermometer N(20, 0.5^2).
    std::cout << "P(T < 19) = Phi(-2) = " << phi((19.0 - 20.0) / 0.5) << "\n";
    std::cout << "P(19.5 < T < 21) = " << phi(2.0) - phi(-1.0) << "\n";
    // Fridge alarm: wrong threshold 4 + 3 * 0.25 = 4.75 is 1.5 sigma; right one is 5.5.
    std::cout << "wrong threshold z = " << (4.75 - 4.0) / 0.5 << ", expected alarms per 10000 = "
              << 10000 * (1.0 - phi(1.5)) << "\n";
    std::cout << "right threshold 5.5, expected alarms per 10000 = " << 10000 * (1.0 - phi(3.0))
              << "\n";
    // Sum of two independent Gaussians; mean of 4 readings.
    std::cout << "sum of N(1, 0.3^2) and N(2, 0.4^2): mean 3, sd "
              << std::sqrt(0.09 + 0.16) << "\n";
    std::cout << "mean of 4 readings with sd 0.5: sd " << 0.5 / std::sqrt(4.0) << "\n";
    // Check-yourself: z of 21.2 in N(20, 0.5^2); a 99.7 % band.
    std::cout << "z of 21.2 = " << (21.2 - 20.0) / 0.5 << ", P(T > 21.2) = "
              << 1.0 - phi(2.4) << "\n";
    std::cout << "3-sigma band for N(20, 0.5^2): " << 20.0 - 1.5 << " .. " << 20.0 + 1.5 << "\n";
    return 0;
}
