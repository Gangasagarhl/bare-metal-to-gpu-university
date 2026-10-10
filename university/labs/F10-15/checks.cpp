// checks.cpp - recomputes the numbers used in F10-15's text, worked example and answer key.
#include <cmath>
#include <cstdio>
#include <numbers>

int main()
{
    const double g = 9.81, deg = std::numbers::pi / 180.0;
    // Worked example: one scalar GNSS position update.
    const double Pp = 0.08 * 0.08, R = 0.5 * 0.5, y = 10.62;
    const double S = Pp + R, K = Pp / S;
    std::printf("W1 P- %.4f m^2, R %.4f m^2: S %.4f, K %.5f, correction %.3f m, ratio %.2f, "
                "P+ %.6f m^2\n",
                Pp, R, S, K, K * y, y / std::sqrt(S), (1 - K) * Pp);
    std::printf("W2 tilt error 1 deg: accel error %.4f m/s^2; velocity error after 1 s %.3f m/s, "
                "after 0.5 s %.3f m/s\n",
                g * std::sin(deg), g * std::sin(deg), 0.5 * g * std::sin(deg));
    std::printf("W3 position error from 1 deg over 2 s: %.3f m\n", 0.5 * g * std::sin(deg) * 4);
    std::printf("W4 ratio threshold 5 sigma with S = %.4f: |innovation| above %.3f m is rejected\n",
                S, 5 * std::sqrt(S));
    std::printf("W5 glitch 12 m against sigma %.3f m: %.1f sigma\n", std::sqrt(S),
                12 / std::sqrt(S));
    std::printf("W6 yaw error 1 deg while accelerating 3 m/s^2: sideways accel error %.4f m/s^2; "
                "in hover (0 m/s^2): 0\n",
                3.0 * std::sin(deg));
    std::printf("Q2 P- 0.04 m^2, R 0.25 m^2: K = %.4f\n", 0.04 / (0.04 + 0.25));
    std::printf("Q4 tilt error 2 deg: accel error %.3f m/s^2, velocity error after 1 s %.3f m/s\n",
                g * std::sin(2 * deg), g * std::sin(2 * deg));
    std::printf("Q6 tilt error 0.5 deg, GNSS lost 10 s: position error %.2f m\n",
                0.5 * g * std::sin(0.5 * deg) * 100);
    return 0;
}
