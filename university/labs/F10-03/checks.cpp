// checks.cpp - recomputes the numbers used in the text of F10-03.
#include <cmath>
#include <cstdio>
#include <initializer_list>
#include <numbers>

int main()
{
    const double g = 9.81, m = 1.0, deg = std::numbers::pi / 180.0;
    const double pitch = std::atan(2.0 / g);
    std::printf("forward accel 2 m/s^2: pitch %.2f deg, thrust %.3f N\n", pitch / deg,
                m * g / std::cos(pitch));
    for (double yawDeg : {0.0, 90.0}) {
        const double y = yawDeg * deg;
        std::printf("pitch %.2f deg, yaw %.0f deg: thrust direction (%.4f, %.4f, %.4f)\n",
                    pitch / deg, yawDeg, std::sin(pitch) * std::cos(y), std::sin(pitch) * std::sin(y),
                    std::cos(pitch));
    }
    const double r = 10 * deg, p = -20 * deg;
    std::printf("roll 10, pitch -20: cos r cos p = %.5f, hover thrust %.4f N\n",
                std::cos(r) * std::cos(p), m * g / (std::cos(r) * std::cos(p)));
    std::printf("roll 20, pitch 20: hover thrust %.4f N\n",
                m * g / (std::cos(20 * deg) * std::cos(20 * deg)));
    std::printf("roll 30 only: hover thrust %.4f N, sideways accel %.4f m/s^2\n",
                m * g / std::cos(30 * deg), g * std::tan(30 * deg));
    // Euler-rate question: level attitude, body rates (0, 0, 1) -> yaw rate 1
    // pitch 60 deg, roll 0, body r = 1 rad/s -> yaw rate 1/cos(60) = 2
    std::printf("pitch 60, roll 0, body z rate 1 rad/s: yaw rate %.4f, roll rate %.4f rad/s\n",
                1.0 / std::cos(60 * deg), std::tan(60 * deg));
    return 0;
}
