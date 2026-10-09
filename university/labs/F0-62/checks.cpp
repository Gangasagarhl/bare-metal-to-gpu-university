// F0-62 checks: recompute every number used in the chapter text.
#include <cmath>
#include <iostream>
#include <vector>

int main()
{
    // Worked example: kitchen scale with a 500 g reference weight (exercise data).
    const std::vector<double> r{512, 509, 511, 510, 513, 509, 511, 510};
    double m = 0.0;
    for (double v : r) {
        m += v / static_cast<double>(r.size());
    }
    double ss = 0.0;
    for (double v : r) {
        ss += (v - m) * (v - m);
    }
    const double sd = std::sqrt(ss / (static_cast<double>(r.size()) - 1.0));
    std::cout << "scale: mean " << m << " g, bias " << m - 500.0 << " g, sd " << sd
              << " g, standard error " << sd / std::sqrt(static_cast<double>(r.size())) << " g\n";
    std::cout << "corrected reading of 734 g displayed: " << 734 - (m - 500.0) << " g\n";
    std::cout << "readings to average for 0.25 g noise: " << std::pow(sd / 0.25, 2) << "\n";
    // Simulator theory values.
    std::cout << "ax: sd theory sqrt(0.02^2 + 0.015^2 + 0.005^2/12) = "
              << std::sqrt(0.0004 + 0.000225 + 0.000025 / 12.0) << "\n";
    std::cout << "r(ax, ay) theory = 0.015^2 / (0.02^2 + 0.015^2) = "
              << 0.000225 / (0.0004 + 0.000225) << "\n";
    std::cout << "spread of r for independent axes, n = 1000: about 1/sqrt(n) = "
              << 1.0 / std::sqrt(1000.0) << "\n";
    std::cout << "quantisation sd for step 0.005: " << 0.005 / std::sqrt(12.0)
              << ", for step 0.06: " << 0.06 / std::sqrt(12.0) << "\n";
    // Gyro: uncorrected bias integrates into heading.
    std::cout << "heading error after 60 s from 0.30 deg/s bias: " << 0.30 * 60 << " deg\n";
    std::cout << "residual bias after a good 10 s calibration (sd 0.1, n 1000): about "
              << 0.1 / std::sqrt(1000.0) << " deg/s -> " << 0.1 / std::sqrt(1000.0) * 60
              << " deg per minute\n";
    std::cout << "forensic: effective error 1.5729 - 0.2967 = " << 1.5729 - 0.296679
              << " deg/s -> " << (1.5729 - 0.296679) * 60 << " deg in 60 s\n";
    // Check yourself: SNR-style ratio and averaging.
    std::cout << "sd 0.03 averaged over 9: " << 0.03 / 3.0 << "\n";
    return 0;
}
