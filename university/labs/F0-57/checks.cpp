// F0-57 checks: recompute every number used in the chapter text.
#include <cmath>
#include <iostream>

double choose(int n, int k)
{
    double result = 1.0;
    for (int i = 1; i <= k; ++i) {
        result = result * (n - k + i) / i;
    }
    return result;
}

int main()
{
    // X = heads in 4 fair tosses: PMF 1, 4, 6, 4, 1 over 16.
    std::cout << "P(X >= 3) = (4 + 1)/16 = " << 5.0 / 16.0 << "\n";
    std::cout << "P(1 <= X <= 3) = F(3) - F(0) = 15/16 - 1/16 = " << 14.0 / 16.0 << "\n";
    std::cout << "P(X = 2) = C(4,2)/16 = " << choose(4, 2) / 16.0 << "\n";
    // Binomial n = 10, p = 0.5.
    std::cout << "P(Y = 5) for n = 10 = C(10,5)/1024 = " << choose(10, 5) / 1024.0 << "\n";
    std::cout << "P(Y = 0) for n = 10 = 1/1024 = " << 1.0 / 1024.0 << "\n";
    // A biased coin (exercise): p = 0.3, n = 4.
    std::cout << "p = 0.3, n = 4: P(exactly 2) = 6 * 0.09 * 0.49 = "
              << choose(4, 2) * std::pow(0.3, 2) * std::pow(0.7, 2) << "\n";
    // Washing machine finishes uniformly in [0, 20] minutes from now.
    std::cout << "uniform on [0, 20]: density = 1/20 = " << 1.0 / 20.0
              << ", P(T < 5) = " << 5.0 / 20.0 << ", P(8 < T < 14) = " << 6.0 / 20.0 << "\n";
    // Density of S = U1 + U2 at s = 0.5 and 1.5 (triangle): s for s < 1, 2 - s for s >= 1.
    std::cout << "triangle density at 0.5: " << 0.5 << ", at 1.5: " << 2.0 - 1.5 << "\n";
    // Kofi's histogram: 8 bins of integer width over 0..1023.
    std::cout << "integer width 1023 / 8 = " << 1023 / 8
              << ", last covered code = " << 8 * (1023 / 8) - 1 << "\n";
    std::cout << "correct width for codes 0..1023 in 8 bins = " << 1024 / 8 << "\n";
    // Dice total PMF sums to 1.
    std::cout << "sum of dice PMF = (1+2+3+4+5+6+5+4+3+2+1)/36 = "
              << (1 + 2 + 3 + 4 + 5 + 6 + 5 + 4 + 3 + 2 + 1) / 36.0 << "\n";
    return 0;
}
