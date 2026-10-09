// F0-53 number check: recomputes every number used in the chapter text.
#include <cmath>
#include <iomanip>
#include <iostream>
#include <numbers>

int main()
{
    std::cout << std::fixed << std::setprecision(4);
    const double pi = std::numbers::pi;
    const double c = std::cos(pi / 6), s = std::sin(pi / 6);
    // worked example: robot at (2,1), heading 30 deg; chair seen at (2,0) in robot frame
    std::cout << "R(30)(2,0) = (" << 2 * c << ", " << 2 * s << "); + (2,1) = (" << 2 * c + 2 << ", " << 2 * s + 1
              << ")\n";
    // back to robot frame
    const double dx = 2 * c, dy = 2 * s;
    std::cout << "R^T((3.7321,2) - (2,1)) = (" << c * dx + s * dy << ", " << -s * dx + c * dy << ")\n";
    // transposed by mistake: R instead of R^T
    std::cout << "R((3.7321,2) - (2,1)) = (" << c * dx - s * dy << ", " << s * dx + c * dy << ")\n";
    // goal (4,3) bearing at start
    std::cout << "bearing of (4,3) from (0,0) = " << std::atan2(3.0, 4.0) * 180 / pi << " deg\n";
    // check yourself
    std::cout << "robot at (1,2) heading 90: point (3,0) in B -> W = (" << 1 + 0 * 3 << ", " << 2 + 3 << ")\n";
    std::cout << "robot at (0,0) heading 180: point (1,1) in B -> W = (-1, -1)\n";
    std::cout << "W point (5,5), robot at (2,1) heading 90: p_B = R^T((3,4)) = (" << 0 * 3 + 1 * 4 << ", "
              << -1 * 3 + 0 * 4 << ")\n";
    const double c45 = std::cos(pi / 4), s45 = std::sin(pi / 4);
    std::cout << "heading 45, p_B = (2,0): R(45)(2,0) = (" << 2 * c45 << ", " << 2 * s45 << ")\n";
    std::cout << "range 3 at bearing 90 from robot at (1,1) heading 0: p_B = (0,3), p_W = (1,4)\n";
    std::cout << "2 x 30 deg = 60 deg; bug bearing error = 2 x heading\n";
    // true bearings from the logged (rounded) poses of the forensic lab
    const double rows[3][3] = {{0.19, 0.06, 18.43}, {0.34, 0.20, 41.35}, {0.43, 0.38, 64.27}};
    for (const auto& r : rows) {
        const double worldAngle = std::atan2(3.0 - r[1], 4.0 - r[0]) * 180 / pi;
        std::cout << "pose (" << r[0] << ", " << r[1] << ", " << r[2] << "): world angle to goal "
                  << worldAngle << ", true bearing " << worldAngle - r[2] << ", world angle + heading "
                  << worldAngle + r[2] << "\n";
    }
    return 0;
}
