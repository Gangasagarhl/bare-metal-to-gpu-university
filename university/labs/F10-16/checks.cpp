// checks.cpp - recomputes the numbers used in F10-16's text, worked example and answer key.
#include <cmath>
#include <cstdio>
#include <numbers>

int main()
{
    const double g = 9.81, m = 1.0, deg = std::numbers::pi / 180.0;
    const double ax = 2.0, Fx = m * ax, Fz = m * g, F = std::hypot(Fx, Fz);
    std::printf("W1 a_sp (2, 0, 0), yaw 0: |F| %.3f N, pitch %.2f deg, roll 0\n", F,
                std::atan2(Fx, Fz) / deg);
    // Yaw 90 degrees: rotate F into the heading frame: Fh = (F.y, -F.x, F.z).
    const double Fhy = -Fx;
    std::printf("W2 same, yaw 90 deg: roll %.2f deg, pitch 0\n", std::asin(-Fhy / F) / deg);
    std::printf("W3 tilt limit 35 deg: max horizontal accel %.3f m/s^2\n", g * std::tan(35 * deg));
    const double r[4] = {0.097, 0.211, 0.566, 1.103}; // copied from loop_steps.out
    std::printf("W4 rise-time ratios: att/rate %.2f, vel/att %.2f, pos/vel %.2f\n", r[1] / r[0],
                r[2] / r[1], r[3] / r[2]);
    std::printf(
        "W5 first-order bandwidth from rise time (2.2/rise): rate %.1f, att %.1f, vel %.1f, "
        "pos %.1f rad/s\n",
        2.2 / r[0], 2.2 / r[1], 2.2 / r[2], 2.2 / r[3]);
    std::printf(
        "K1 wobble: posP 4.5 rad/s against velocity-loop bandwidth %.1f rad/s (ratio %.2f)\n",
        2.2 / r[2], 4.5 / (2.2 / r[2]));
    const double peaks[4] = {1.2, 3.4, 5.2, 7.2}; // times of x peaks in wobble.out
    std::printf("K2 wobble period from peaks: %.2f s (%.2f Hz)\n", (peaks[3] - peaks[0]) / 3,
                3 / (peaks[3] - peaks[0]));
    std::printf("K3 attitude-loop rate setpoint for a 10 deg error: %.3f rad/s (attP 6)\n",
                6.0 * 2 * std::sin(5 * deg));

    std::printf("Q2 a_sp (0, 1.5, 0), yaw 0: roll %.2f deg, |F| %.3f N (1 kg)\n",
                std::asin(-1.5 / std::hypot(1.5, g)) / deg, std::hypot(1.5, g));
    std::printf("Q4 tilt limit 25 deg: max horizontal accel %.2f m/s^2\n", g * std::tan(25 * deg));
    std::printf("Q6 2 kg vehicle, a_sp (2, 0, 0): |F| %.3f N, pitch %.2f deg\n",
                2.0 * std::hypot(2.0, g), std::atan2(2.0, g) / deg);
    return 0;
}
