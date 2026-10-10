// F9-34 checks: numbers quoted in the chapter (linearisation error, angle wrapping).
#include <cmath>
#include <iomanip>
#include <iostream>

int main()
{
    const double pi = std::acos(-1.0);
    std::cout << std::fixed << std::setprecision(4);
    // Worked example: robot at (1, 2), heading 0.7 rad; landmark at (4, 4).
    const double dx = 3.0, dy = 2.0, q = dx * dx + dy * dy;
    std::cout << "range " << std::sqrt(q) << " m, bearing " << std::atan2(dy, dx) - 0.7 << " rad\n";
    std::cout << "H = [" << -dx / std::sqrt(q) << ", " << -dy / std::sqrt(q) << ", 0; " << dy / q
              << ", " << -dx / q << ", -1]\n";
    // Linearisation error of the bearing for a sideways error of d metres.
    for (double d : {0.1, 0.5, 2.0}) {
        const double exact = std::atan2(dy - d, dx) - std::atan2(dy, dx);  // robot moves +d in y
        const double linear = (-dx / q) * d;
        std::cout << "robot shifted " << d << " m in y: bearing change exact " << exact
                  << ", linear " << linear << ", error " << exact - linear << " rad\n";
    }
    // Angle wrapping.
    const double raw = 3.10 - (-3.10);
    std::cout << "raw difference 3.10 - (-3.10) = " << raw << ", wrapped = "
              << raw - 2.0 * pi << " rad\n";
    // The forensic first jump: the unwrapped innovation 6.323 rad, wrapped.
    std::cout << "6.323 - 2 pi = " << 6.323 - 2.0 * pi << " rad\n";
    // Mean of a function versus function of the mean: E[cos(th)] for th ~ N(0, s^2).
    for (double sdDeg : {2.0, 10.0, 30.0}) {
        const double s = sdDeg * pi / 180.0;
        std::cout << "heading sd " << sdDeg << " deg: cos(mean) = 1, E[cos] = "
                  << std::exp(-s * s / 2)
                  << '\n';
    }
    return 0;
}
