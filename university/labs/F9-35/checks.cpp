// F9-35 checks: numbers quoted in the chapter, recomputed.
#include <cmath>
#include <iomanip>
#include <iostream>

int main()
{
    const double pi = std::acos(-1.0);
    const double deg = pi / 180.0;
    std::cout << std::fixed << std::setprecision(4);
    std::cout << "gyro bias 0.3 deg/s over 120 s: " << 0.3 * 120.0 << " deg\n";
    std::cout << "slip 1: 0.6 rad/s for 1.5 s = " << 0.6 * 1.5 << " rad = " << 0.9 / deg
              << " deg; slip 2: 0.6 rad/s for 1.0 s = " << 0.6 / deg << " deg; total "
              << (0.9 + 0.6) / deg << " deg\n";
    std::cout << "speed slip 0.15 m/s for 2.5 s = " << 0.15 * 2.5 << " m\n";
    std::cout << "0.3 deg/s in rad/s = " << 0.3 * deg << '\n';
    std::cout << "variance ratio odometry/gyro = (0.02/0.00174)^2 = "
              << std::pow(0.02 / 0.00174, 2) << '\n';
    std::cout << "standard error of a 200-sample mean innovation (sd 0.02) = "
              << 0.02 / std::sqrt(200.0) << " rad/s\n";
    std::cout << "a -0.0052 rad/s mean is " << 0.0052 / (0.02 / std::sqrt(200.0))
              << " standard errors from zero\n";
    // NIS of one slip reading if S is about the odometry variance (0.02^2).
    std::cout << "NIS of a 0.6 rad/s discrepancy with S = 0.0004: " << 0.36 / 0.0004 << '\n';
    // Gate of 9 for one degree of freedom = 3 sd; share of good readings rejected.
    std::cout << "P(NIS > 9) for a consistent 1-dof innovation = "
              << std::erfc(3.0 / std::sqrt(2.0))
              << " (expected false rejections in 1200 updates: "
              << 1200.0 * std::erfc(3.0 / std::sqrt(2.0)) << ")\n";
    std::cout << "heading error at the dock in the forensic run: 104.4 - 68.8 = " << 104.4 - 68.8
              << " deg; 0.3 deg/s x 120 s = 36 deg\n";
    return 0;
}
