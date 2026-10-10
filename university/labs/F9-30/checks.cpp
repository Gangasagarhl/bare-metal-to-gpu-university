// F9-30 checks: recomputes every number used in the chapter's text from formulas.
#include <cmath>
#include <iomanip>
#include <iostream>

int main()
{
    std::cout << std::fixed << std::setprecision(4);
    // Variance added by one move of Listing 1: 0.1*(-1)^2 + 0.8*0^2 + 0.1*(+1)^2 around +1.
    const double perMove = 0.1 * 1.0 + 0.8 * 0.0 + 0.1 * 1.0;
    std::cout << "variance added per move: " << perMove << '\n';
    for (int k = 1; k <= 4; ++k) {
        std::cout << "  sd after " << k << " moves = sqrt(" << perMove * k
                  << ") = " << std::sqrt(perMove * k) << '\n';
    }
    // Two-peak belief of Listing 1, part B (mean 9).
    const double var2 = 2.0 * (0.1 * 36.0 + 0.3 * 25.0 + 0.1 * 16.0);
    std::cout << "two-peak variance " << var2 << ", sd " << std::sqrt(var2) << '\n';
    // Listing 2 part B: expected x = sum_k exp(-k s^2 / 2) for heading random walk.
    const double s = 3.0 * std::acos(-1.0) / 180.0;
    double ex = 0.0;
    for (int k = 1; k <= 20; ++k) {
        ex += std::exp(-k * s * s / 2.0);
    }
    std::cout << "theory: mean x after 20 steps with 3 deg heading noise = " << ex << " m\n";
    // Heading sd after 20 steps and a small-angle estimate of the lateral sd.
    std::cout << "heading sd after 20 steps = " << s * std::sqrt(20.0) * 180.0 / std::acos(-1.0)
              << " deg\n";
    double lateralVar = 0.0;
    for (int k = 1; k <= 20; ++k) {
        // y = sum_k sin(theta_k) ~ sum_k theta_k; theta_k = sum_{j<=k} n_j.
        // Each n_j appears in (20 - j + 1) terms.
        lateralVar += static_cast<double>((20 - k + 1) * (20 - k + 1)) * s * s;
    }
    std::cout << "small-angle lateral sd after 20 steps = " << std::sqrt(lateralVar) << " m\n";
    // Forensic: real sd after k steps and the share of |error| > 0.1 m.
    for (int k : {1, 4, 25}) {
        const double sd = 0.05 * std::sqrt(static_cast<double>(k));
        const double share = std::erfc(0.1 / sd / std::sqrt(2.0));
        std::cout << "k = " << k << ": true sd " << sd << " m, P(|error| > 0.1 m) = "
                  << 100.0 * share << " %\n";
    }
    // Grid size for (x, y, theta) on 20 m x 20 m at 5 cm and 5 degrees, and its memory.
    const double cells = (20.0 / 0.05) * (20.0 / 0.05) * (360.0 / 5.0);
    std::cout << "cells: " << cells << ", bytes as double: " << cells * 8.0 << '\n';
    // Check-yourself 6: motion model 0.2, 0.6, 0.2 -> variance 0.4 per move.
    std::cout << "sd after 4 moves with 0.2/0.6/0.2: " << std::sqrt(4.0 * 0.4) << '\n';
    // Sampling error of an sd estimated from N samples: about 1/sqrt(2N) relative.
    std::cout << "relative standard error of an sd from 4000 samples: "
              << 1.0 / std::sqrt(2.0 * 4000.0) << '\n';
    return 0;
}
